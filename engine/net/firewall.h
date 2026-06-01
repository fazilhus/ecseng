//------------------------------------------------------------------------------
// firewall.h
// Helper to programmatically add inbound UDP firewall rules on Windows.
// Uses netsh advfirewall — requires admin rights for first-time setup.
// If not elevated, the call is silently ignored; Windows Firewall will still
// prompt on first socket bind for private-network profiles.
// (C) 2026 Individual contributors
//------------------------------------------------------------------------------
#pragma once

namespace Core {

#ifdef _WIN32
#include <cstdio>
#include <windows.h>

inline void ensure_udp_firewall_rule(const char* name)
{
    char exe[MAX_PATH];
    if (GetModuleFileNameA(nullptr, exe, MAX_PATH) == 0)
        return;

    char cmd[1024];

    // Check if the rule already exists
    snprintf(cmd, sizeof(cmd),
        "netsh advfirewall firewall show rule name=\"%s\" >nul 2>&1", name);
    if (system(cmd) == 0)
        return;

    // Add inbound UDP allow rule for this executable
    snprintf(cmd, sizeof(cmd),
        "netsh advfirewall firewall add rule name=\"%s\" dir=in action=allow "
        "protocol=UDP program=\"%s\" enable=yes >nul 2>&1",
        name, exe);
    system(cmd);
}

#else
// Non-Windows: no-op
inline void ensure_udp_firewall_rule(const char*) {}
#endif

} // namespace Core
