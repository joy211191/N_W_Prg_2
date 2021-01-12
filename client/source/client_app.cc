// client_app.cc

#include "client_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>

template <typename T, std::size_t N>
constexpr auto array_size(T(&)[N])
{
   return N;
}

ClientApp::ClientApp()
    : mouse_(window_.mouse_)
    , keyboard_(window_.keyboard_)
    , tickrate_(1.0 / 60.0)
    , input_bits_(0),
    clientTick(0),
    currentServerTick(0)
{
    player_.playerID = 999;
    synced = false;
}

bool ClientApp::on_init()
{
   network_.set_send_rate(Time(1.0 / 20.0));
   if (!network_.initialize({})) {
      return false;
   }

   connection_.set_listener(this);
   //connection_.connect(network::IPAddress(127, 0, 0, 1, 54345));

   return true;
}

void ClientApp::on_exit()
{
}

bool ClientApp::on_tick(const Time &dt)
{
   if (keyboard_.pressed(Keyboard::Key::Escape)) {
      return false;
   }
   if (!serverFound)
       serverFound = ServerDiscovery();
   if (serverFound) {
       if (keyboard_.pressed(Keyboard::Key::Space) && (connection_.state_ == network::Connection::State::Invalid || connection_.is_disconnected())) {
           connection_.connect(serverIP);
       }
   }

   if (connection_.is_connected()) {
       accumulator_ += dt;
       while (accumulator_ >= tickrate_) {
           accumulator_ -= tickrate_;
           clientTick++;
           //printf("Client tick: %d,Server tick: %d,Offset tick: %d\n", (int)clientTick,(int)currentServerTick,(int)offsetTick);
           EntityInterpolation();
           GetInput();
           PlayerPosition();
       }
   }
   return true;
}

void ClientApp::GetInput()
{
    input_bits_ = 0;
    if (keyboard_.down(Keyboard::Key::W)) {
        input_bits_ |= (1 << int32(gameplay::Action::Up));
    }
    if (keyboard_.down(Keyboard::Key::S)) {
        input_bits_ |= (1 << int32(gameplay::Action::Down));
    }
    if (keyboard_.down(Keyboard::Key::A)) {
        input_bits_ |= (1 << int32(gameplay::Action::Left));
    }
    if (keyboard_.down(Keyboard::Key::D)) {
        input_bits_ |= (1 << int32(gameplay::Action::Right));
    }
}

void ClientApp::PlayerPosition()
{
    if (synced) {
        gameplay::Inputinator temp;
        temp.inputBits = input_bits_;
        temp.tick = clientTick;
        temp.calculatedPosition = player_.position_ + GetInputDirection(input_bits_) * SPEED * tickrate_.as_seconds();
        player_.inputLibrary.push_back(temp);
        player_.position_ += GetInputDirection(input_bits_) * SPEED * tickrate_.as_seconds();
        if (player_.inputLibrary.size() > offsetTick) {
            auto inpt = player_.inputLibrary.begin();
            inpt += offsetTick;
            player_.inputLibrary.erase(player_.inputLibrary.begin(), inpt);
        }
    }
}

void ClientApp::EntityInterpolation()
{
    for (int32 i = 0; i < otherPlayers.size(); i++) {
        float distance = Vector2::distance(otherPlayers[i].newPosition, otherPlayers[i].position_);
        otherPlayers[i].position_ = Vector2::lerp(otherPlayers[i].position_, otherPlayers[i].newPosition, SPEED * distance * tickrate_.as_milliseconds());
    }
}

Vector2 ClientApp::GetInputDirection(uint8 input)
{
    const bool player_move_up = input & (1 << int32(gameplay::Action::Up));
    const bool player_move_down = input & (1 << int32(gameplay::Action::Down));
    const bool player_move_left = input & (1 << int32(gameplay::Action::Left));
    const bool player_move_right = input & (1 << int32(gameplay::Action::Right));

    Vector2 inputDirection = Vector2::Zero;

    if (player_move_up) {
        inputDirection.y_ -= 1.0f;
    }
    if (player_move_down) {
        inputDirection.y_ += 1.0f;
    }
    if (player_move_left) {
        inputDirection.x_ -= 1.0f;
    }
    if (player_move_right) {
        inputDirection.x_ += 1.0f;
    }
    inputDirection.normalize();
    return inputDirection;
}

void ClientApp::on_draw()
{
   renderer_.clear({ 0.2f, 0.3f, 0.4f, 1.0f });
   renderer_.render_text({ 2, 2 }, Color::White, 1, "CLIENT");
   if (serverFound&& (connection_.state_ == network::Connection::State::Invalid || connection_.is_disconnected())) {
       renderer_.render_text_va({ 2,2 }, Color::Yellow, 1, "Server found at: %s", serverIP.as_string());
       renderer_.render_text({ 3,12 }, Color::Red, 1, "Do you want to connect? ");
   }
   else  {
       //printf("Other players list: %d\n", (int)otherPlayers.size());
       if (otherPlayers.size() > 0) {
           auto en = otherPlayers.begin();
           while (en != otherPlayers.end()) {
               renderer_.render_rectangle_fill({ int32((*en).position_.x_), int32((*en).position_.y_), 20, 20 },Color::White );//(*en).playerColor
               ++en;
           }
       }
       renderer_.render_rectangle_fill({ int32(player_.position_.x_), int32(player_.position_.y_), 20, 20 }, player_.playerColor);
   }
}

void ClientApp::on_acknowledge(network::Connection *connection, const uint16 sequence)
{

}

void ClientApp::on_receive(network::Connection* connection, network::NetworkStreamReader& reader)
{
    while (reader.position() < reader.length()) {
        switch (reader.peek()) {
            case network::NETWORK_MESSAGE_PLAYERID: {
                if (player_.playerID > 8) {
                    network::NetworkPlayerSetupID message;
                    if(!message.read(reader))
                        assert(!"could not read message!");
                    player_.playerID = message.playerID_;
                    printf("My id=%d\n", (int)player_.playerID);
                }
                break;
            }
            case network::NETWORK_MESSAGE_SERVER_TICK:
            {
                network::NetworkMessageServerTick message;
                if (!message.read(reader)) {
                    assert(!"could not read message!");
                }

                const Time current = Time(message.server_time_);
                currentServerTick = message.server_tick_;
                uint32 offset = currentServerTick - clientTick;
                uint32 latency = (uint32)((connection->latency().as_milliseconds() / tickrate_.as_milliseconds()) * 2);
                uint32 sendRate = (uint32)(Time(1.0 / 20.0).as_milliseconds());
                offsetTick = offset + latency + sendRate;
                Synchronize(currentServerTick);
                //printf("Offset tick: %d\n Server tick: %d\n Client tick: %d" ,(int)offsetTick,(int)currentServerTick,(int)clientTick);

                break;
            }
            case network::NETWORK_MESSAGE_ENTITY_STATE:
            {
                network::NetworkMessageEntityState message;
                //printf("Data recieved for player with id: %d\n", (int)message.id_);
                if (!message.read(reader)) {
                    assert(!"could not read message!");
                }
                auto otherPlayer = otherPlayers.begin();
                bool playerExists = false;
                while (otherPlayer != otherPlayers.end())
                {
                    if (message.id_ == (*otherPlayer).id) {
                        playerExists = true;
                        break;
                    }
                    ++otherPlayer;
                }
                if (!playerExists) {
                    gameplay::Entity entity;
                    entity.id = message.id_;
                    entity.newPosition - message.position_;
                    otherPlayers.push_back(entity);
                    //printf("Other players list size: %d\n", (int)otherPlayers.size());
                }
                else
                    (*otherPlayer).newPosition = message.position_;
                break;
            }
            case network::NETWORK_MESSAGE_PLAYER_STATE:
            {
                network::NetworkMessagePlayerState message;
                if (!message.read(reader)) {
                    assert(!"could not read message!");
                }
                CheckPlayerPosition(message.tick_, message.position_);
                break;
            }
            default:
            {
                assert(!"unknown message type received from server!");
                break;
            }
        }
    }
}

void ClientApp::Synchronize(uint32 serverTick)
{
    clientTick = serverTick;
    synced = true;
}

void ClientApp::CheckPlayerPosition(uint32 serverTick, Vector2 serverPosition)
{
    auto in = player_.inputLibrary.begin();
    while (in != player_.inputLibrary.end()) {
        if ((*in).tick==serverTick-offsetTick) {
            printf("Distance:%f\n", Vector2::distance(serverPosition, (*in).calculatedPosition));
            if (Vector2::distance(serverPosition, (*in).calculatedPosition) > 5) {
                networkData.inputMispredictions++;
                FixPlayerPositions(serverTick, serverPosition);
            }
            else {
                player_.position_ = (*in).calculatedPosition;
            }
            player_.inputLibrary.erase(player_.inputLibrary.begin(), in);
            break;
        }
        ++in;
    }
}

void ClientApp::FixPlayerPositions(uint32 serverTick, Vector2 serverPosition)
{
    auto in = player_.inputLibrary.begin();
    while (in != player_.inputLibrary.end()) {
        if ((*in).tick == serverTick-offsetTick) {
            (*in).calculatedPosition = serverPosition;
            break;
        }
        if ((*in).tick < serverTick) {
            Vector2 dir = GetInputDirection((*in).inputBits);
            if (dir.length() > 0.0f) {
                dir.normalize();
                serverPosition += dir;
            }
            (*in).calculatedPosition = serverPosition;
        }
        ++in;
    }
    player_.position_ = serverPosition;
}

void ClientApp::on_send(network::Connection *connection, const uint16 sequence, network::NetworkStreamWriter &writer)
{
   network::NetworkMessageInputCommand command(input_bits_,clientTick,offsetTick);
   if (!command.write(writer)) {
      assert(!"could not write network command!");
   }
}

bool ClientApp::ServerDiscovery() {
    if (!socket.is_valid())
        socket.open();
    if (socket.is_valid()) {
        DynamicArray<network::IPAddress> addresses;
        network::IPAddress address;
        if (network::IPAddress::local_addresses(addresses)) {
            for (auto& ad : addresses)
                address = ad;
        }
        address.host_ = (0xffffff00 & address.host_) | 0xff;
        address.port_ = 54345;

        network::NetworkStream stream;
        network::NetworkStreamWriter writer(stream);
        network::ProtocolRequestPacket packet;

        if (packet.write(writer)) {
            if (socket.send(address, stream.buffer_, stream.length_))
                std::printf("Ping sent\n");
        }

        bool found = false;
        int tries = 0;

        while (!found) {
            stream.reset();
            if (socket.receive(address, stream.buffer_, stream.length_)) {
                network::NetworkStreamReader reader(stream);
                if (reader.peek() == network::ProtocolPacketType::PROTOCOL_PACKET_CHALLENGE) {
                    std::printf("Server found\n");
                    serverIP = address;
                    found = true;
                    return true;
                }
            }
            else {
                tries++;
                auto error_code = network::Error::get_last();
                if (network::Error::is_critical(error_code)) {
                    assert(false);
                    break;
                }
                if (packet.write(writer)) {
                    if (socket.send(address, stream.buffer_, stream.length_)) {
                        error_code = network::Error::get_last();
                        if (network::Error::is_critical(error_code)) {
                            assert(false);
                            break;
                        }
                    }
                }
            }
            if (tries > 10) break;
            Time::sleep(Time(1.0 / 50.0));
        }
        std::printf("Out of tries\n");

        auto error_code = network::Error::get_last();
        std::printf("ERR: %d\n", error_code);
        if (network::Error::is_critical(error_code)) {
            assert(false);
        }
        return false;
    }
    else
        return false;
}

bool ClientApp::ConnectionCheck()
{
    if (connection_.state_ != network::Connection::State::Connected)
        return false;
    else
        return true;
}

void ConnectionHandler::on_rejected(const uint8 reason) {
    std::printf("Lobby full, %d", reason);
}