// client_app.hpp

#ifndef CLIENT_APP_HPP_INCLUDED
#define CLIENT_APP_HPP_INCLUDED
constexpr float SPEED = 100;

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
   Vector2 GetInputDirection(uint8 input);
   void EntityInterpolation(uint32 id, Vector2 newPosition);
   virtual void on_draw();

   // note: IConnectionListener 
   virtual void on_acknowledge(network::Connection *connection, const uint16 sequence);
   virtual void on_receive(network::Connection *connection, network::NetworkStreamReader &reader);
   void CheckPlayerPosition(uint32 serverTick, Vector2 serverPosition);
   void FixPlayerPositions(uint32 serverTick, Vector2 serverPosition);
   virtual void on_send(network::Connection *connection, const uint16 sequence, network::NetworkStreamWriter &writer);

   Mouse &mouse_;
   Keyboard &keyboard_;
   network::Connection connection_;
   const Time tickrate_;
   Time accumulator_;

   uint32 clientTick;
   uint32 currentServerTick;
   uint32 offsetTick;
   uint8 input_bits_;
   gameplay::Player player_;
   charlie::DynamicArray<gameplay::Entity> otherPlayers;
   bool idApplied;
   int winnerID;

   network::UDPSocket socket;
   network::IPAddress serverIP;
   bool serverFound = false;
   bool ServerDiscovery();
   bool ConnectionCheck();
   struct DataStruct {
   public:
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
};

#endif // !CLIENT_APP_HPP_INCLUDED
