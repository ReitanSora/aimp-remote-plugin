#pragma once

/**
 * @file Config.h
 * @brief Compile-time server configuration values used across the application.
 *
 * This header exposes small, strongly-typed, compile-time constants for
 * configuring the HTTP and WebSocket listeners. These values are intended
 * to be changed at build time; changing them requires recompilation.
 */

namespace Config
{
    // =========================================================================
    // Server Configuration
    // =========================================================================
    /**
     * @brief Host address the HTTP server will bind to.
     *
     * Typical values:
     * - "0.0.0.0"  : bind to all IPv4 interfaces (default)
     * - "127.0.0.1": bind to localhost only
     * - "::"       : bind to all IPv6 interfaces (may require an IPv6-capable socket)
     *
     * Note: This is a null-terminated string literal and a compile-time constant.
     */
    constexpr const char* HTTP_HOST = "0.0.0.0";

    /**
     * @brief TCP port for the HTTP server.
     *
     * Valid range: 1 - 65535. Using ports < 1024 may require elevated privileges
     * depending on the operating system.
     *
     * This is a compile-time integer constant.
     */
    constexpr int HTTP_PORT = 3553;
    
    /**
     * @brief Host address the WebSocket server will bind to.
     *
     * See `HTTP_HOST` for typical values and notes about IPv4/IPv6 bindings.
     */
    constexpr const char* WEBSOCKET_HOST = "0.0.0.0";

    /**
     * @brief TCP port for the WebSocket server.
     *
     * Valid range: 1 - 65535. Ensure the port does not conflict with other services.
     */
    constexpr int WEBSOCKET_PORT = 3554;
}