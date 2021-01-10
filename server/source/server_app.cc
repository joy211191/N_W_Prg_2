// server_app.cc

#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>

ServerApp::ServerApp()
   : tickrate_(1.0 / 60.0)
   , tick_(0)
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

bool ServerApp::on_tick(const Time &dt)
{
   accumulator_ += dt;
   while (accumulator_ >= tickrate_) {
      accumulator_ -= tickrate_;
      tick_++;
      auto pl = players.begin();
      printf("%d\n",(int)players.size());
     while (pl != players.end()) {
          uint8 tempInput = 0;
          auto in = (*pl).inputLibrary.begin();
          while (in!=(*pl).inputLibrary.end())
          {
              if ((*in).tick == tick_) {
                  tempInput = (*in).inputBits;
                  (*pl).inputLibrary.erase((*pl).inputLibrary.begin(), in);
                  break;
              }
              ++in;
          }
          // note: update player
          const bool player_move_up = tempInput & (1 << int32(gameplay::Action::Up));
          const bool player_move_down = tempInput & (1 << int32(gameplay::Action::Down));
          const bool player_move_left = tempInput & (1 << int32(gameplay::Action::Left));
          const bool player_move_right = tempInput & (1 << int32(gameplay::Action::Right));
          Vector2 direction;
          if (player_move_up) {
              direction.y_ -= 1.0f;
          }
          if (player_move_down) {
              direction.y_ += 1.0f;
          }
          if (player_move_left) {
              direction.x_ -= 1.0f;
          }
          if (player_move_right) {
              direction.x_ += 1.0f;
          }
          const float speed = 100.0;
          if (direction.length() > 0.0f) {
              direction.normalize();
              (*pl).position_ += direction * speed * tickrate_.as_seconds();
          }
          ++pl;
      }

      //entity_[0].position_.x_ = 300.0f + std::cosf(Time::now().as_seconds()) * 150.0f;
      //entity_[0].position_.y_ = 180.0f + std::sinf(Time::now().as_seconds()) * 100.0f;

      //entity_[1].position_.x_ = 150.0f + std::cosf(Time::now().as_seconds()) * 75.0f;
      //entity_[1].position_.y_ = 90.0f + std::sinf(Time::now().as_seconds()) * 50.0f;
   }

   return true;
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
   //renderer_.render_rectangle_fill({ static_cast<int32>(entity_[0].position_.x_), static_cast<int32>(entity_[0].position_.y_),  20, 20 }, Color::Green);
}

void ServerApp::on_timeout(network::Connection *connection)
{
   connection->set_listener(nullptr);
   auto id = clients_.find_client((uint64)connection);
   // ...
   clients_.remove_client((uint64)connection);
}

void ServerApp::on_connect(network::Connection *connection)
{
   connection->set_listener(this);
   auto id = clients_.add_client((uint64)connection);
   newConnection = true;
   newConnectionID = clients_.find_client((uint64)connection);
   gameplay::Player newPlayer;
   newPlayer.playerID = newConnectionID;
   newPlayer.alive = true;
   players.push_back(newPlayer);
   printf("On connect runs\n");
}

void ServerApp::on_disconnect(network::Connection *connection)
{
   connection->set_listener(nullptr);
   auto id = clients_.find_client((uint64)connection);
   // ...
   clients_.remove_client((uint64)connection); 
   auto pl = players.begin();
   while (1) {
       if ((*pl).playerID == id) {
           players.erase(pl);
           break;
       }
   }
}

void ServerApp::on_acknowledge(network::Connection *connection, const uint16 sequence)
{

}

void ServerApp::on_receive(network::Connection *connection, network::NetworkStreamReader &reader)
{
   auto id = clients_.find_client((uint64)connection);
   printf("Recieveing info\n");
   while (reader.position() < reader.length()) {
      if (reader.peek() != network::NETWORK_MESSAGE_INPUT_COMMAND) {
         break;
      }

      network::NetworkMessageInputCommand command;
      if (!command.read(reader)) {
         assert(!"could not read command!");
      }
      gameplay::Inputinator temp;
      temp.inputBits = command.bits_;
      temp.tick = command.tick_;
      for (int i = 0; i < players.size(); i++) {
          if (i == id) {
              players[i].inputLibrary.push_back(temp);
              break;
          }
      }
   }
}

void ServerApp::on_send(network::Connection* connection, const uint16 sequence, network::NetworkStreamWriter& writer)
{
    auto pl = players.begin();
    while (pl != players.end()) {
        if (newConnection) {
            network::NetowrkNewConnection message(clients_.find_client(newConnectionID));
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }
        {
            network::NetworkMessageServerTick message(Time::now().as_ticks(), tick_);
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }

        auto id = clients_.find_client((uint64)connection);
        //players_[id]. ..;
        if (id == (*pl).playerID)
        {
            network::NetworkMessagePlayerState message(tick_, (*pl).position_);
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }
        else
        {
            network::NetworkMessageEntityState message(tick_, (*pl).position_);
            if (!message.write(writer)) {
                assert(!"failed to write message!");
            }
        }
        ++pl;
    }
    printf("Sending as well\n");
}