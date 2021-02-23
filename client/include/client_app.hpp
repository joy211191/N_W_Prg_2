// client_app.hpp

#ifndef CLIENT_APP_HPP_INCLUDED
#define CLIENT_APP_HPP_INCLUDED
constexpr float PLAYER_SPEED = 100;
constexpr float BULLET_SPEED = 150;

#include <charlie_application.hpp>
#include <charlie_protocol.hpp>

using namespace charlie;

class ConnectionHandler :public network::Connection {
public:
	void on_rejected(const uint8 reason);


private:
};



struct ClientApp final : Application, network::IConnectionListener {
   ClientApp();

   // note: Application
   virtual bool on_init();
   virtual void on_exit();
   virtual bool on_tick(const Time &dt);
   void Bullet();
   void GetInput();
   void PlayerPosition();
   Vector2 GetInputDirection(uint8 input);
   void Bullet(const bool& player_shoot, const charlie::Vector2& inputDirection);
   void EntityInterpolation();
   void ClearBuffer(DynamicArray<gameplay::Bullet> entityList);
   void ClearBuffer(DynamicArray<gameplay::Entity> entityList);
   virtual void on_draw();

   // note: IConnectionListener 
   virtual void on_acknowledge(network::Connection *connection, const uint16 sequence);
   virtual void on_receive(network::Connection *connection, network::NetworkStreamReader &reader);
   void CheckPlayerPosition(uint32 serverTick, Vector2 serverPosition);
   void FixPlayerPositions(uint32 serverTick, Vector2 serverPosition);
   virtual void on_send(network::Connection *connection, const uint16 sequence, network::NetworkStreamWriter &writer);
   void Synchronize(uint32 serverTick);

   Mouse &mouse_;
   Keyboard &keyboard_;
   network::Connection connection_;
   const Time tickrate_;
   Time accumulator_;

   uint32 clientTick;
   uint32 currentServerTick;
   uint32 offsetTick;
   uint8 input_bits_;
   gameplay::Player player;
   gameplay::Bullet playerBullet;
   charlie::DynamicArray<gameplay::Bullet> otherBullets;
   charlie::DynamicArray<gameplay::Entity> otherPlayers;
   bool idApplied;
   int winnerID;

   bool synced;
   uint32 lastSentTick;

   network::UDPSocket socket;
   network::IPAddress serverIP;
   bool serverFound = false;
   bool ServerDiscovery();
   bool ConnectionCheck();
   struct DataStruct {
   public:
	   Time currentServerTime;
	   Time lastSent;
	   Time LastRecieved;
	   Time RTT;
	   int packetsSent;
	   int packetsDelivered;
	   int packetsLost;
	   int packetsReceived;
	   int sequenceNumber;
	   float packetLoss;
	   bool detailsOverlay;
	   charlie::DynamicArray<int> sequenceStack;
	   int32 dataSize;
	   int32 inputMispredictions;
   };
   DataStruct networkData;

   enum EventType {
	   PLAYER_ID,
	   PLAYER_DEATH,
	   PLAYER_SHOOT,
   };

   struct Event {
	   uint32 playerID;
	   EventType event;
	   uint16 sequenceNumber;
   };

   charlie::DynamicArray<Event> eventQueue;
};

#endif // !CLIENT_APP_HPP_INCLUDED
