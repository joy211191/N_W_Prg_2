// server_app.hpp

#ifndef SERVER_APP_HPP_INCLUDED
#define SERVER_APP_HPP_INCLUDED
constexpr float PLAYER_SPEED = 100;

#include <charlie_application.hpp>

using namespace charlie;

struct ClientList {
   ClientList()
      : next_(0)
   {
   }

   int32 add_client(const uint64 connection)
   {
      const int32 id = next_++;
      clients_.push_back({ id, connection });
      return id;
   }

   int32 find_client(const uint64 connection)
   {
      for (auto &client : clients_) {
         if (client.connection_ == connection) {
            return client.id_;
         }
      }
      return -1;
   }

   void remove_client(const uint64 connection)
   {
      auto it = clients_.begin();
      while (it != clients_.end()) {
         if ((*it).connection_ == connection) {
            clients_.erase(it);
            break;
         }
         ++it;
      }
   }

   struct Client {
      int32  id_{ -1 };
      uint64 connection_{};
   };

   int32 next_;
   DynamicArray<Client> clients_;
};

struct ServerApp final : Application, network::IServiceListener, network::IConnectionListener {
   ServerApp();

   // note: Application
   virtual bool on_init();
   virtual void on_exit();
   virtual bool on_tick(const Time &dt);
   Vector2 GetInputDirection(uint8 input,uint32 playerID);
   virtual void on_draw();

   // note: IServiceListener
   virtual void on_timeout(network::Connection *connection);
   virtual void on_connect(network::Connection *connection);
   virtual void on_disconnect(network::Connection *connection);

   // note: IConnectionListener 
   virtual void on_acknowledge(network::Connection *connection, const uint16 sequence);
   virtual void on_receive(network::Connection *connection, network::NetworkStreamReader &reader);
   virtual void on_send(network::Connection *connection, const uint16 sequence, network::NetworkStreamWriter &writer);



   const Time tickrate_;
   Time accumulator_;
   uint32 serverTick;
   ClientList clients_;

   charlie::DynamicArray<gameplay::Bullet>bullets;
   Vector2 send_position_;

   Random random_;
   charlie::DynamicArray<gameplay::Player>players;
   uint8 player_input_bits_;

   bool newConnection;
   int32 newConnectionID;

   Vector2 direction;
   uint8 tempInput;
};

#endif // !SERVER_APP_HPP_INCLUDED
