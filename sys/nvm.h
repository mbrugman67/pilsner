#ifndef NVM_H_
#define NVM_H_

#include <string>

class nvm
{
public:
    nvm();
    ~nvm() {}

    void write();
    void load();

    void setTZ(int t);
    void setSSID(const std::string& s);
    void setPwd(const std::string& p);

    int getTZ()                             { return (nvmData.tz); }
    const std::string getSSID() const       { return (std::string(nvmData.ssid)); }
    const std::string getPwd() const        { return (std::string(nvmData.pw)); }
private:
    struct nvm_t
    {
        uint32_t signature;
        char ssid[64];
        char pw[64];

        int tz;
    } nvmData;
};

#endif // NVM_H_