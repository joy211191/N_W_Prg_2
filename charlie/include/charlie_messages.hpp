// charlie_messages.hpp

#ifndef CHARLIE_MESSAGES_HPP_INCLUDED
#define CHARLIE_MESSAGES_HPP_INCLUDED

#include <charlie.hpp>

namespace charlie {
   namespace network {
      struct NetworkStreamReader;
      struct NetworkStreamWriter;

      enum NetworkMessageType {
         NETWORK_MESSAGE_SERVER_TICK,
         NETWORK_MESSAGE_ENTITY_STATE,
         NETWORK_MESSAGE_INPUT_COMMAND,
         NETWORK_MESSAGE_PLAYER_STATE,
         NETWORK_MESSAGE_COUNT,
         NETWORK_NEW_CONNECTION,
      };

      static_assert(NETWORK_MESSAGE_COUNT <= 255, "network message type cannot exceed 255!");

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

      struct NetowrkNewConnection {
          NetowrkNewConnection();
          explicit NetowrkNewConnection(const int32 playerId);

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
         explicit NetworkMessageInputCommand(uint8 bits,uint32 tick);

         bool read(NetworkStreamReader &reader);
         bool write(NetworkStreamWriter &writer);

         template <typename Stream>
         bool serialize(Stream &stream)
         {
            bool result = true;
            result &= stream.serialize(type_);
            result &= stream.serialize(bits_);
            result &= stream.serialize(tick_);
            return result;
         }

         uint8 type_;
         uint8 bits_;
         uint32 tick_;
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
