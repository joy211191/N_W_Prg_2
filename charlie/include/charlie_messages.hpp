// charlie_messages.hpp

#ifndef CHARLIE_MESSAGES_HPP_INCLUDED
#define CHARLIE_MESSAGES_HPP_INCLUDED

#include <charlie.hpp>

namespace charlie {
   namespace network {
      struct NetworkStreamReader;
      struct NetworkStreamWriter;

      enum NetworkMessageType {
          NETWORK_NEW_CONNECTION,
          NETWORK_MESSAGE_SERVER_TICK,
          NETWORK_MESSAGE_ENTITY_STATE,
          NETWORK_MESSAGE_INPUT_COMMAND,
          NETWORK_MESSAGE_PLAYER_STATE,
          NETWORK_MESSAGE_COUNT,
          NETWORK_MESSAGE_PLAYERID,
          NETWORK_MESSAGE_SHOOT,
      };

      static_assert(NETWORK_MESSAGE_COUNT <= 255, "network message type cannot exceed 255!");

      struct NetworkMessageShoot {
          NetworkMessageShoot();
          explicit NetworkMessageShoot(const uint8 active, const uint32 server_tick, const uint32 id, const Vector2 position, const Vector2 shootDirection);

          bool read(NetworkStreamReader& reader);
          bool write(NetworkStreamWriter& writer);

          template <typename Stream>
          bool serialize(Stream& stream) {
              bool result = true;
              result &= stream.serialize(type_);
              result &= stream.serialize(bulletActive);
              result &= stream.serialize(server_tick_);
              result &= stream.serialize(playerID);
              result &= stream.serialize(bulletPosition.x_);
              result &= stream.serialize(bulletPosition.y_);
              result &= stream.serialize(direction.x_);
              result &= stream.serialize(direction.y_);
              return result;
          }

          uint8 type_;
          uint8 bulletActive;
          uint32 server_tick_;
          uint32 playerID;
          Vector2 bulletPosition;
          Vector2 direction;
      };

      struct NetworkMessageServerTick {
         NetworkMessageServerTick();
         explicit NetworkMessageServerTick(const int64  server_time, const uint32 server_tick);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(server_time_);
            result &= stream.serialize(server_tick_);
            return result;
         }

         uint8 type_;
         int64 server_time_;
         uint32 server_tick_;
      };

      struct NetworkPlayerSetupID {
          NetworkPlayerSetupID();
          explicit NetworkPlayerSetupID(const int32 playerId);

          bool read(NetworkStreamReader& reader);
          bool write(NetworkStreamWriter& writer);

          template <typename Stream>
          bool serialize(Stream& stream)
          {
              bool result = true;
              result &= stream.serialize(type_);
              result &= stream.serialize(playerID_);
              return result;
          }

          uint8 type_;
          int32 playerID_;
      };

      struct NetworkMessageEntityState {
         NetworkMessageEntityState();
         explicit NetworkMessageEntityState(uint32 id, const Vector2 &position);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(position_.x_);
            result &= stream.serialize(position_.y_);
            result &= stream.serialize(id_);
            return result;
         }
         uint32 id_;
         uint8 type_;
         Vector2 position_;
      };

      struct NetworkMessageInputCommand {
         NetworkMessageInputCommand();
         explicit NetworkMessageInputCommand(uint8 bits,uint32 tick,uint32 offsetTick);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(bits_);
            result &= stream.serialize(tick_);
            result &= stream.serialize(offsetTick_);
            return result;
         }

         uint8 type_;
         uint8 bits_;
         uint32 tick_;
         uint32 offsetTick_;
      };

      struct NetworkMessagePlayerState {
         NetworkMessagePlayerState();
         explicit NetworkMessagePlayerState(uint32 tick, const Vector2 &position);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(position_.x_);
            result &= stream.serialize(position_.y_);
            result &= stream.serialize(tick_);
            return result;
         }

         uint8 type_;
         Vector2 position_;
         uint32 tick_;
      };
   } // !network
} // !charlie

#endif // !CHARLIE_MESSAGES_HPP_INCLUDED
