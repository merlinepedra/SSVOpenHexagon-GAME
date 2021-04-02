// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/TcpSocket.hpp>
#include <SFML/Network/Packet.hpp>

#include <list>
#include <cstdint>
#include <chrono>

namespace hg::Steam
{

class steam_manager;

}

namespace hg
{

class HexagonClient
{
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    Steam::steam_manager& _steamManager;

    std::uint64_t _ticketSteamID;

    const sf::IpAddress _serverIp;
    const unsigned short _serverPort;

    sf::TcpSocket _socket;
    bool _socketConnected;

    sf::Packet _packetBuffer;

    TimePoint _lastHeartbeatTime;

    [[nodiscard]] bool initializeTicketSteamID();
    [[nodiscard]] bool initializeTcpSocket();

    [[nodiscard]] bool sendHeartbeat();

    void disconnect();

    bool sendHeartbeatIfNecessary();

public:
    explicit HexagonClient(Steam::steam_manager& steamManager);
    ~HexagonClient();

    HexagonClient(const HexagonClient&) = delete;
    HexagonClient(HexagonClient&&) = delete;

    void update();
};

} // namespace hg
