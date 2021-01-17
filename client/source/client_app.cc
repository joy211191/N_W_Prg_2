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
    , tickrate_(1.0 / 20.0)
    , input_bits_(0),
    clientTick(0),
    currentServerTick(0)
{
    player.playerID = 999;
    synced = false;
    playerBullet.active = false;
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
           printf("Client tick: %d,Server tick: %d,Offset tick: %d\n", (int)clientTick,(int)currentServerTick,(int)offsetTick);
           EntityInterpolation();
           GetInput();
           PlayerPosition();
           Bullet();
       }
   }
   return true;
}

void ClientApp::Bullet()
{
    //printf("Bullet direction X:%f Y:%f\n", playerBullet.direction.x_, playerBullet.direction.y_);
    if (playerBullet.active) {
        playerBullet.position_ += playerBullet.direction * 150 * tickrate_.as_seconds();
        //printf("Bullet moving\n");
    }

    if (playerBullet.position_.x_<0 || playerBullet.position_.x_>window_.width_ || playerBullet.position_.y_<0 || playerBullet.position_.y_>window_.height_)
        playerBullet.active = false;
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
    if (keyboard_.down(Keyboard::Key::Space)) {
        input_bits_ |= (1 << int32(gameplay::Action::Shoot));
    }
}

void ClientApp::PlayerPosition()
{
    if (synced) {
        gameplay::Inputinator temp;
        temp.inputBits = input_bits_;
        temp.tick = clientTick;
        temp.calculatedPosition = player.position_ + GetInputDirection(input_bits_) * PLAYER_SPEED * tickrate_.as_seconds();
        player.inputLibrary.push_back(temp);
        player.position_ += GetInputDirection(input_bits_) * PLAYER_SPEED * tickrate_.as_seconds();
        /*if (player.inputLibrary.size() > offsetTick*2) {
            auto inpt = player.inputLibrary.begin();
            inpt += offsetTick;
            player.inputLibrary.erase(player.inputLibrary.begin(), inpt);
        }*/
    }
}

void ClientApp::EntityInterpolation()
{
    for (int32 i = 0; i < otherPlayers.size(); i++) {
        float distance = Vector2::distance(otherPlayers[i].newPosition, otherPlayers[i].position_);
        otherPlayers[i].position_ = Vector2::lerp(otherPlayers[i].position_, otherPlayers[i].newPosition, PLAYER_SPEED * distance * tickrate_.as_milliseconds());
    }
    for(int32 i=0;i<otherBullets.size();i++){
        float distance = Vector2::distance(otherBullets[i].recievedPosition, otherBullets[i].position_);
        otherBullets[i].position_ = Vector2::lerp(otherBullets[i].position_, otherBullets[i].recievedPosition, PLAYER_SPEED * distance * tickrate_.as_milliseconds());
    }
}

Vector2 ClientApp::GetInputDirection(uint8 input)
{
    const bool player_move_up = input & (1 << int32(gameplay::Action::Up));
    const bool player_move_down = input & (1 << int32(gameplay::Action::Down));
    const bool player_move_left = input & (1 << int32(gameplay::Action::Left));
    const bool player_move_right = input & (1 << int32(gameplay::Action::Right));
    const bool player_shoot = input & (1 << int32(gameplay::Action::Shoot));

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
    Bullet(player_shoot, inputDirection);
    return inputDirection;
}

void ClientApp::Bullet(const bool& player_shoot, const charlie::Vector2& inputDirection)
{
    if (inputDirection.length() > 0) {
        if (!playerBullet.active && player_shoot) {
            playerBullet.active = true;
            Vector2 offset = Vector2(10, 10);
            playerBullet.position_ = player.position_ + offset;
            playerBullet.direction = inputDirection;
        }
        else {
            playerBullet.position_ = playerBullet.position_;
        }
    }
}

void ClientApp::on_draw()
{
    renderer_.clear({ 0.2f, 0.3f, 0.4f, 1.0f });
    renderer_.render_text({ 2, 2 }, Color::White, 1, "CLIENT");
    if (serverFound && (connection_.state_ == network::Connection::State::Invalid || connection_.is_disconnected())) {
        renderer_.render_text_va({ 2,2 }, Color::Yellow, 1, "Server found at: %s", serverIP.as_string());
        renderer_.render_text({ 3,12 }, Color::Red, 1, "Do you want to connect? ");
    }
    else {
        if (otherPlayers.size() > 0) {
            auto en = otherPlayers.begin();
            while (en != otherPlayers.end()) {
                renderer_.render_rectangle_fill({ int32((*en).position_.x_), int32((*en).position_.y_), 20, 20 }, Color::White);
                ++en;
            }
            auto bl = otherBullets.begin();
            while (bl != otherBullets.end()) {
                if ((*bl).active)
                    renderer_.render_rectangle_fill({ int32((*bl).position_.x_), int32((*bl).position_.y_), 10, 10 }, Color::White);
                bl;
            }
        }
        renderer_.render_rectangle_fill({ int32(player.position_.x_), int32(player.position_.y_), 20, 20 }, player.playerColor);
        if (playerBullet.active)
            renderer_.render_rectangle_fill({ int32(playerBullet.position_.x_), int32(playerBullet.position_.y_), 10, 10 }, playerBullet.bulletColor);
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
                if (player.playerID > 8) {
                    network::NetworkPlayerSetupID message;
                    if(!message.read(reader))
                        assert(!"could not read message!");
                    player.playerID = message.playerID_;
                    playerBullet.bulletID = message.playerID_;
                }
                break;
            }
            case network::NETWORK_MESSAGE_SHOOT: {
                network::NetworkMessageShoot message;
                if (!message.read(reader)) {
                    assert(!"could not read message!");
                }
                if (message.playerID == player.playerID) {
                    break;
                    /*playerBullet.active = message.bulletActive;
                    playerBullet.active = message.bulletActive;
                    playerBullet.position_ = message.bulletPosition;*/
                }
                else {
                    auto bullet = otherBullets.begin();
                    while (bullet != otherBullets.end())
                    {
                        if ((*bullet).bulletID == message.playerID) {
                            (*bullet).active =message.bulletActive;
                            (*bullet).recievedPosition = message.bulletPosition;
                            break;
                        }
                        ++bullet;
                    }
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
                uint32 latency = (uint32)(connection_.latency().as_milliseconds() / tickrate_.as_milliseconds());
                offsetTick = (uint32)(latency * 0.06f) + 5;
                Synchronize(currentServerTick);
                break;
            }
            case network::NETWORK_MESSAGE_ENTITY_STATE:
            {
                network::NetworkMessageEntityState message;
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
                    gameplay::Bullet bullet;
                    bullet.active = false;
                    bullet.bulletID = message.id_;
                    bullet.position_ = message.position_;
                    bullet.bulletColor = entity.playerColor;
                    otherBullets.push_back(bullet);
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
    clientTick = serverTick+offsetTick;
    lastSentTick = clientTick;
    synced = true;
}

void ClientApp::CheckPlayerPosition(uint32 serverTick, Vector2 serverPosition)
{
    auto in = player.inputLibrary.begin();
    while (in != player.inputLibrary.end()) {
        if ((*in).tick==serverTick) {
            if (Vector2::distance(serverPosition, (*in).calculatedPosition) > 1) {
                networkData.inputMispredictions++;
                FixPlayerPositions(serverTick, serverPosition);
                break;
            }
            else {
                player.position_ = (*in).calculatedPosition;
            }
            player.inputLibrary.erase(player.inputLibrary.begin(), in);
        }
        ++in;
    }
}

void ClientApp::FixPlayerPositions(uint32 serverTick, Vector2 serverPosition)
{
    auto in = player.inputLibrary.begin();
    while (in != player.inputLibrary.end()) {
        if ((*in).tick == serverTick) {
            (*in).calculatedPosition = serverPosition;
            break;
        }
        if ((*in).tick < serverTick) {
            player.inputLibrary.erase(in);
        }
        ++in;
    }
    player.position_ = serverPosition;
}

void ClientApp::on_send(network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer)
{
    network::NetworkMessageInputCommand command(input_bits_, clientTick, offsetTick);
    if (!command.write(writer)) {
        assert(!"could not write network command!");
    }
    network::NetworkMessageShoot shoot(playerBullet.active, currentServerTick, playerBullet.bulletID, playerBullet.position_, playerBullet.direction);
    if (!shoot.write(writer)) {
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