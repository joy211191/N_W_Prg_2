// charlie_messages.cc

#include "charlie_messages.hpp"
#include "charlie_network.hpp"

namespace charlie {
   namespace network {

       NetworkMessageShoot::NetworkMessageShoot() {}

       NetworkMessageShoot::NetworkMessageShoot(const uint8  active, const uint32 server_tick, const uint32 id, const Vector2 position, const Vector2 shootDirection)
           :type_(NETWORK_MESSAGE_SHOOT),
           bulletActive(active),
           server_tick_(server_tick),
           playerID(id),
           bulletPosition(position),
           direction(shootDirection)
       {

       }

       bool NetworkMessageShoot::read(NetworkStreamReader& reader) {
           return serialize(reader);
       }

       bool NetworkMessageShoot::write(NetworkStreamWriter& writer) {
           return serialize(writer);
       }

      NetworkMessageServerTick::NetworkMessageServerTick()
         : type_(NETWORK_MESSAGE_SERVER_TICK)
         , server_time_(0)
         , server_tick_(0)
      {
      }

      NetworkMessageServerTick::NetworkMessageServerTick(const int64  server_time,
                                                         const uint32 server_tick)
         : type_(NETWORK_MESSAGE_SERVER_TICK)
         , server_time_(server_time)
         , server_tick_(server_tick)
      {
      }

      bool NetworkMessageServerTick::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessageServerTick::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }

#pragma region PLAYER_ID

      NetworkPlayerSetupID::NetworkPlayerSetupID()
          : type_(NETWORK_MESSAGE_PLAYERID)
          , playerID_(0)
      {
      }

      NetworkPlayerSetupID::NetworkPlayerSetupID(int32 playerID)
          : type_(NETWORK_MESSAGE_PLAYERID)
          , playerID_(playerID)
      {
      }

      bool NetworkPlayerSetupID::read(NetworkStreamReader& reader)
      {
          return serialize(reader);
      }

      bool NetworkPlayerSetupID::write(NetworkStreamWriter& writer)
      {
          return serialize(writer);
      }

#pragma endregion

      NetworkMessageEntityState::NetworkMessageEntityState()
         : type_(NETWORK_MESSAGE_ENTITY_STATE),
          position_(Vector2::Zero),
          id_(0)
      {
      }

      NetworkMessageEntityState::NetworkMessageEntityState(uint32 id,const Vector2 &position)
         : type_(NETWORK_MESSAGE_ENTITY_STATE)
         , position_(position),
          id_(id)
      {
      }

      bool NetworkMessageEntityState::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessageEntityState::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }

      NetworkMessageInputCommand::NetworkMessageInputCommand()
          : type_(NETWORK_MESSAGE_INPUT_COMMAND)
          , bits_(0),
          tick_(0),
          offsetTick_(0)
      {
      }

      NetworkMessageInputCommand::NetworkMessageInputCommand(uint8 bits,uint32 tick,uint32 offsetTick)
         : type_(NETWORK_MESSAGE_INPUT_COMMAND)
         , bits_(bits),
          tick_(tick),
          offsetTick_(offsetTick)
      {
      }

      bool NetworkMessageInputCommand::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessageInputCommand::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }
     
      NetworkMessagePlayerState::NetworkMessagePlayerState()
         : type_(NETWORK_MESSAGE_PLAYER_STATE)
      {
      }

      NetworkMessagePlayerState::NetworkMessagePlayerState(uint32 tick, const Vector2& position)
          : type_(NETWORK_MESSAGE_PLAYER_STATE)
          , position_(position),
          tick_(tick)
      {
      }

      bool NetworkMessagePlayerState::read(NetworkStreamReader &reader)
      {
         return serialize(reader);
      }

      bool NetworkMessagePlayerState::write(NetworkStreamWriter &writer)
      {
         return serialize(writer);
      }
   } // !network
} // !messages
