// server_app.cc

#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>

ServerApp::ServerApp()
   : tickrate_(1.0 / 20.0)
   , serverTick(0)
   , player_input_bits_(0)
{
}

bool ServerApp::on_init()
{
   network_.set_send_rate(Time(1.0 / 20.0));
   network_.set_allow_connections(true);
   if (!network_.initialize(network::IPAddress(network::IPAddress::ANY_HOST, 54345))) {
      return false;
   }

   network_.add_service_listener(this);

   return true;
}

void ServerApp::on_exit()
{
}

bool ServerApp::on_tick(const Time& dt)
{
    accumulator_ += dt;
    while (accumulator_ >= tickrate_) {
        accumulator_ -= tickrate_;
        serverTick++;
        auto pl = players.begin();
        while (pl != players.end()) {
            tempInput = 0;
            auto in = (*pl).inputLibrary.begin();
            while (in != (*pl).inputLibrary.end())
            {
                if (serverTick == (*in).tick) {
                    tempInput = (*in).inputBits;
                    (*pl).inputLibrary.erase((*pl).inputLibrary.begin(), in);
                    break;
                }
                ++in;
            }
            (*pl).position_ += GetInputDirection(tempInput, (*pl).playerID) * PLAYER_SPEED * tickrate_.as_seconds();
            ++pl;
        }
        auto bl = bullets.begin();
        while (bl != bullets.end())
        {
            Bullet((*bl).bulletID);
            ++bl;
        }
    }
    return true;
}

Vector2 ServerApp::GetInputDirection(uint8 input, uint32 playerID)
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
    players[playerID].shoot = player_shoot;
    inputDirection.normalize();
    direction = inputDirection;
    if (player_shoot) {
        bullets[playerID].direction = direction;
        bullets[playerID].position_ = players[playerID].position_;
        bullets[playerID].active = true;
    }
    return inputDirection;
}

void ServerApp::Bullet(const charlie::uint32& playerID)
{
    bullets[playerID].position_ += bullets[playerID].direction * 150 * tickrate_.as_seconds();
    auto pl = players.begin();
    while (pl != players.end())
    {
        if ((*pl).playerID != bullets[playerID].bulletID) {
            if (CollisionCheck(bullets[playerID].position_, (*pl).position_)) {
                (*pl).alive = false;
                bullets[playerID].active = false;
                break;
            }
        }
        ++pl;
    }
    if (bullets[playerID].position_.x_<0 || bullets[playerID].position_.x_>window_.width_ || bullets[playerID].position_.y_<0 || bullets[playerID].position_.y_>window_.height_)
        bullets[playerID].active = false;
}

void ServerApp::on_draw()
{
    renderer_.clear({ 0.4f, 0.3f, 0.2f, 1.0f });
    renderer_.render_text({ 2, 2 }, Color::White, 2, "SERVER");
    auto pl = players.begin();
    while (pl != players.end()) {
        renderer_.render_rectangle_fill({ static_cast<int32>((*pl).position_.x_), static_cast<int32>((*pl).position_.y_),  20, 20 }, (*pl).playerColor);
        ++pl;
    }
    auto bl = bullets.begin();
    while (bl != bullets.end()) {
        if ((*bl).active)
            renderer_.render_rectangle_fill({ static_cast<int32>((*bl).position_.x_), static_cast<int32>((*bl).position_.y_),  10, 10 }, (*bl).bulletColor);
        ++bl;
    }
}

bool ServerApp::CollisionCheck(Vector2 positionA, Vector2 positionB) {
    if (positionA.x_<positionB.x_ + 20 && positionA.x_ + 20 >positionB.x_ && positionA.y_<positionB.y_ + 20 && positionA.y_ + 20>positionB.y_)
        return true;
    else
        return false;
}

void ServerApp::on_timeout(network::Connection *connection)
{
   connection->set_listener(nullptr);
   auto id = clients_.find_client((uint64)connection);

   clients_.remove_client((uint64)connection);
}

void ServerApp::on_connect(network::Connection *connection)
{
   connection->set_listener(this);
   auto id = clients_.add_client((uint64)connection);
   newConnection = true;
   gameplay::Player newPlayer;
   printf("Connection id: %d\n", (int)id);
   newPlayer.playerID = id;
   newPlayer.alive = true;
   newPlayer.idAssigned = false;
   newPlayer.shoot = false;
   players.push_back(newPlayer);

   gameplay::Bullet newBullet;
   newBullet.active = false;
   newBullet.bulletID = id;
   bullets.push_back(newBullet);
}

void ServerApp::on_disconnect(network::Connection *connection)
{
   connection->set_listener(nullptr);
   auto id =(uint32) clients_.find_client((uint64)connection);

   clients_.remove_client((uint64)connection); 
   auto pl = players.begin();
   while (1) {
       if ((*pl).playerID == id) {
           players.erase(pl);
           break;
       }
       ++pl;
   }
}

void ServerApp::on_acknowledge(network::Connection *connection, const uint16 sequence)
{
    auto id = clients_.find_client((uint64)connection);
    auto eve=players[id].eventQueue.begin();
    while (eve != players[id].eventQueue.end()) {
        if ((*eve).sequenceNumber == sequence) {
            players[id].eventQueue.erase(eve);
            players[id].idAssigned = true;
            break;
        }
        ++eve;
    }
}

void ServerApp::on_receive(network::Connection *connection, network::NetworkStreamReader &reader)
{
   auto id = (uint32)clients_.find_client((uint64)connection);
   while (reader.position() < reader.length()) {
      if (reader.peek() != network::NETWORK_MESSAGE_INPUT_COMMAND) {
         break;
      }
      network::NetworkMessageInputCommand command;
      if (!command.read(reader)) {
         assert(!"could not read command!");
      }

      players[id].offsetTick = command.offsetTick_;
      gameplay::Inputinator temp;
      temp.inputBits = command.bits_;
      temp.tick = command.tick_;
      for (int i = 0; i < players.size(); i++) {
          if (i ==(int) id) {
              players[i].inputLibrary.push_back(temp);
              break;
          }
      }
      network::NetworkMessageShoot shoot;
      if (!shoot.read(reader)) {
         assert(!"could not read command!");
      }
      printf("%d", (int)shoot.bulletActive);
      for (int i = 0; i < bullets.size(); i++) {
          if (shoot.playerID == id) {
              bullets[i].position_ = shoot.bulletPosition;
              bullets[i].active = shoot.bulletActive;
              bullets[i].direction = shoot.direction;
              break;
          }
      }
   }
}

void ServerApp::on_send(network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer)
{
    auto id = (uint32)clients_.find_client((uint64)connection);
    if (!players[id].idAssigned) {
        int32 playerID = (int32)id;
        network::NetworkPlayerSetupID playerIDMessage(playerID);
        if (!playerIDMessage.write(writer)) {
            assert(!"failed to write message!");
        }
        gameplay::PlayerEvent newEvent;
        newEvent.playerID = id;
        newEvent.type = gameplay::PlayerEventTypes::PLAYER_ID;
        newEvent.sequenceNumber = sequence;
        players[id].idAssigned = true;
        players[id].eventQueue.push_back(newEvent);
    }
    auto bl = bullets.begin();
    while (bl != bullets.end())
    {
        network::NetworkMessageShoot message((*bl).active, serverTick, (*bl).bulletID, (*bl).position_, (*bl).direction);
        if (!message.write(writer)) {
            assert(!"failed to write message!");
        }
        
        ++bl;
    }
    auto pl = players.begin();
    while (pl != players.end()) {
        {
            network::NetworkMessageServerTick message(Time::now().as_ticks(), serverTick);
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }
        if (id == (*pl).playerID)
        {
            network::NetworkMessagePlayerState message(serverTick, (*pl).position_,(*pl).alive);
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }
        else
        {
            network::NetworkMessageEntityState message((*pl).playerID, (*pl).position_,(*pl).alive);
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }
        ++pl;
    }
}