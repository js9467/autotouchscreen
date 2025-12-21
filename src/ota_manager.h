#pragma once

#include <cstdint>
#include <string>

struct WifiStatusSnapshot;

class OTAUpdateManager {
public:
    static OTAUpdateManager& instance();

    void begin();
    void loop(const WifiStatusSnapshot& wifi_status);
    void triggerImmediateCheck();
    const std::string& lastStatus() const { return last_status_; }

private:
    OTAUpdateManager() = default;

    struct ManifestInfo {
        std::string version;
        std::string channel;
        std::string firmware_url;
        std::string md5;
        std::uint32_t size = 0;
    };

    bool fetchManifest(ManifestInfo& manifest);
    bool applyManifest(const ManifestInfo& manifest);
    bool downloadAndInstall(const ManifestInfo& manifest);
    bool isNewerVersion(const std::string& remote_version) const;
    static int compareVersions(const std::string& lhs, const std::string& rhs);
    void setStatus(const std::string& status);

    bool enabled_ = false;
    bool auto_apply_ = true;
    std::string manifest_url_;
    std::string expected_channel_ = "stable";
    std::uint32_t check_interval_ms_ = 3600000;  // 60 minutes
    std::uint32_t last_check_ms_ = 0;
    bool wifi_ready_ = false;
    bool pending_manual_check_ = false;
    std::string last_status_ = "idle";
};
