#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "esp_gap_ble_api.h"

#define PASSKEY 130516

// UUID cua dich vu va dac tinh
static BLEUUID ballServiceUUID("00003344-5566-7788-99aa-bbccddeeff00");
static BLEUUID charUUIDBallSpeed("0001483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID charUUIDSideAngle("0002483e-36e1-4688-b7f5-ea07361b26a8");

static BLEUUID clubServiceUUID("00013344-5566-7788-99aa-bbccddeeff00");
static BLEUUID charUUIDClubSpeed("00016284-97e1-4688-b7f5-ea07361b26a8");
static BLEUUID charUUIDAngleOfAttack("00026284-97e1-4688-b7f5-ea07361b26a8");

static BLEUUID imageServiceUUID("7e884490-8d17-43b5-941e-48aa2d8d618a");
static BLEUUID charUUIDImage("00028257-8489-40b2-affb-ad3783c89952");

// Bien toan cuc
static BLEAddress *pServerAddress = nullptr;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic *pRemoteCharBallSpeed = nullptr;
static BLERemoteCharacteristic *pRemoteCharSideAngle = nullptr;
static BLERemoteCharacteristic *pRemoteCharClubSpeed = nullptr;
static BLERemoteCharacteristic *pRemoteCharAngleOfAttack = nullptr;
static BLERemoteCharacteristic *pRemoteCharImage = nullptr;
static BLEClient *pClient = nullptr;

// Bien nhan anh
//#define IMAGE_SIZE (240 * 240 * 2) // RGB565
//uint8_t image_buffer[IMAGE_SIZE];
volatile int image_offset = 0;
bool image_complete = false;

// Xoa cac lien ket bond cu
void clearBondingInfo()
{
    int dev_num = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t dev_list[dev_num];

    if (dev_num > 0 && esp_ble_get_bond_device_list(&dev_num, dev_list) == ESP_OK)
    {
        for (int i = 0; i < dev_num; i++)
        {
            esp_ble_remove_bond_device(dev_list[i].bd_addr);
            Serial.printf("🧹 Removed bonding with device: %02X:%02X:%02X:%02X:%02X:%02X\n",
                          dev_list[i].bd_addr[0], dev_list[i].bd_addr[1], dev_list[i].bd_addr[2],
                          dev_list[i].bd_addr[3], dev_list[i].bd_addr[4], dev_list[i].bd_addr[5]);
        }
    }
    else
    {
        Serial.println("ℹ️ No bonded devices found.");
    }
}

// Ham notify khi co du lieu gui den
static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    Serial.print("📩 Notify from: ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" | Text: ");
    Serial.write(pData, length); // In dữ liệu dạng chuỗi ASCII
    Serial.println();
    // if (image_offset + length <= IMAGE_SIZE) {
    //     memcpy(&image_buffer[image_offset], pData, length);
    //     image_offset += length;
    //     Serial.printf("📥 Received %d/%d bytes\n", image_offset, IMAGE_SIZE);
    // }

    // if (image_offset >= IMAGE_SIZE) {
    //     image_complete = true;
    //     Serial.println("✅ Image fully received!");
    // }
}

// Xac thuc bao mat ket noi
class MySecurity : public BLESecurityCallbacks
{
public:
    uint32_t onPassKeyRequest()
    {
        Serial.println(" Server requested passkey, returning PASSKEY");
        return PASSKEY; // Trả về passkey cho server
    }
    void onPassKeyNotify(uint32_t pass_key)
    {
        Serial.print(" Passkey Notify from server: ");
        Serial.println(pass_key);
    }
    bool onConfirmPIN(uint32_t pin)
    {
        Serial.print(" Confirm PIN from server: ");
        Serial.println(pin);
        if (pin == PASSKEY)
        {                // Kiểm tra PIN có khớp với PASSKEY không
            return true; // Xác nhận passkey nếu khớp
        }
        else
        {
            return false;
        }
    }
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl)
    {
        if (auth_cmpl.success)
        {
            Serial.println("🔓 Authentication Successful!");
            connected = true;
        }
        else
        {
            Serial.print("❌ Authentication Failed! Reason: ");
            Serial.println(auth_cmpl.fail_reason);
            connected = false;
        }
    }
    bool onSecurityRequest()
    {
        Serial.println("🔒 Security Request from Server...");
        return true;
    }
};

// Ham Client chu dong doc du lieu tu Server
void readCharacteristicData()
{
    // Đọc Ball Speed
    if (pRemoteCharBallSpeed && pRemoteCharBallSpeed->canRead())
    {
        std::string value = pRemoteCharBallSpeed->readValue();
        Serial.print("🏀 Ball Speed (READ): ");
        if (value.empty())
        {
            Serial.println("No data returned (empty)");
        }
        else
        {
            Serial.print("Length: ");
            Serial.print(value.length());
            Serial.print(", Value: ");
            Serial.println(value.c_str()); // Chuyển std::string -> const char*
        }
    }
    else
    {
        Serial.println("Ball Speed read fail");
    }

    // Đọc Side Angle
    if (pRemoteCharSideAngle && pRemoteCharSideAngle->canRead())
    {
        std::string value = pRemoteCharSideAngle->readValue();
        Serial.print("📐 Side Angle (READ): ");
        if (value.empty())
        {
            Serial.println("No data returned (empty)");
        }
        else
        {
            Serial.print("Length: ");
            Serial.print(value.length());
            Serial.print(", Value: ");
            Serial.println(value.c_str()); // Chuyển std::string -> const char*
        }
    }
    else
    {
        Serial.println("Side Angle read fail");
    }

    // Đọc Club Speed
    if (pRemoteCharClubSpeed && pRemoteCharClubSpeed->canRead())
    {
        std::string value = pRemoteCharClubSpeed->readValue();
        Serial.print("🏌️ Club Speed (READ): ");
        if (value.empty())
        {
            Serial.println("No data returned (empty)");
        }
        else
        {
            Serial.print("Length: ");
            Serial.print(value.length());
            Serial.print(", Value: ");
            Serial.println(value.c_str()); // Chuyển std::string -> const char*
        }
    }
    else
    {
        Serial.println("Club Speed read fail");
    }

    // Đọc Angle of Attack
    if (pRemoteCharAngleOfAttack && pRemoteCharAngleOfAttack->canRead())
    {
        std::string value = pRemoteCharAngleOfAttack->readValue();
        Serial.print("⛳ Angle of Attack (READ): ");
        if (value.empty())
        {
            Serial.println("No data returned (empty)");
        }
        else
        {
            Serial.print("Length: ");
            Serial.print(value.length());
            Serial.print(", Value: ");
            Serial.println(value.c_str()); // Chuyển std::string -> const char*
        }
    }
    else
    {
        Serial.println("Angle of Attack read fail");
    }
}

// Ham ket noi voi Server
bool connectToServer(BLEAddress pAddress)
{
    Serial.print("🔗 Connecting to ");
    Serial.println(pAddress.toString().c_str());

    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
    BLEDevice::setSecurityCallbacks(new MySecurity());

    BLESecurity *pSecurity = new BLESecurity();
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_BOND_MITM); // Fix
    pSecurity->setCapability(ESP_IO_CAP_KBDISP);                 // Fix
    pSecurity->setKeySize(16);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

    if (pClient == nullptr)
    {
        pClient = BLEDevice::createClient();
        Serial.println(" - Created client");
    }

    if (!pClient->connect(pAddress))
    {
        Serial.println("❌ Failed to connect to server");
        return false;
    }
    Serial.println(" - Connected to server");

    // Đợi pairing hoàn tất
    delay(3000);

    // Lấy dịch vụ Ball Information
    BLERemoteService *pRemoteService = pClient->getService(ballServiceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("❌ Failed to find Ball Information service UUID: ");
        Serial.println(ballServiceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found Ball Information service");

    // Lấy đặc tính Ball Speed
    pRemoteCharBallSpeed = pRemoteService->getCharacteristic(charUUIDBallSpeed);
    if (pRemoteCharBallSpeed == nullptr)
    {
        Serial.print("❌ Failed to find Ball Speed characteristic UUID: ");
        Serial.println(charUUIDBallSpeed.toString().c_str());
        pClient->disconnect();
        return false;
    }
    else
    {
        Serial.println(" - Found Ball Speed characteristic");
        if (pRemoteCharBallSpeed->canNotify())
        {
            pRemoteCharBallSpeed->registerForNotify(notifyCallback);
            Serial.println("📡 Registered for Notify on Ball Speed");
        }
    }

    // Lấy đặc tính Side Angle
    pRemoteCharSideAngle = pRemoteService->getCharacteristic(charUUIDSideAngle);
    if (pRemoteCharSideAngle == nullptr)
    {
        Serial.print("❌ Failed to find Side Angle characteristic UUID: ");
        Serial.println(charUUIDSideAngle.toString().c_str());
        pClient->disconnect();
        return false;
    }
    else
    {
        Serial.println(" - Found Side Angle characteristic");
        if (pRemoteCharSideAngle->canNotify())
        {
            pRemoteCharSideAngle->registerForNotify(notifyCallback);
            Serial.println("📡 Registered for Notify on Side Angle");
        }
    }

    // Lấy dịch vụ Club Information
    BLERemoteService *pClubService = pClient->getService(clubServiceUUID);
    if (pClubService == nullptr)
    {
        Serial.print("❌ Failed to find Club Information service UUID: ");
        Serial.println(clubServiceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found Club Information service");

    // Lấy đặc tính Club Speed
    pRemoteCharClubSpeed = pClubService->getCharacteristic(charUUIDClubSpeed);
    if (pRemoteCharClubSpeed == nullptr)
    {
        Serial.print("❌ Failed to find Club Speed characteristic UUID: ");
        Serial.println(charUUIDClubSpeed.toString().c_str());
        pClient->disconnect();
        return false;
    }
    else
    {
        Serial.println(" - Found Club Speed characteristic");
        if (pRemoteCharClubSpeed->canNotify())
        {
            pRemoteCharClubSpeed->registerForNotify(notifyCallback);
            Serial.println("📡 Registered for Notify on Club Speed");
        }
    }

    // Lấy đặc tính Angle of Attack
    // pRemoteCharAngleOfAttack = pClubService->getCharacteristic(charUUIDAngleOfAttack);
    // if (pRemoteCharAngleOfAttack == nullptr)
    // {
    //     Serial.print("❌ Failed to find Angle of Attack characteristic UUID: ");
    //     Serial.println(charUUIDAngleOfAttack.toString().c_str());
    //     pClient->disconnect();
    //     return false;
    // }
    // else
    // {
    //     Serial.println(" - Found Angle of Attack characteristic");
    //     if (pRemoteCharAngleOfAttack->canNotify())
    //     {
    //         pRemoteCharAngleOfAttack->registerForNotify(notifyCallback);
    //         Serial.println("📡 Registered for Notify on Angle of Attack");
    //     }
    // }

    // Lấy dịch vụ Image
    BLERemoteService *pImageService = pClient->getService(imageServiceUUID);
    if (pImageService == nullptr)
    {
        Serial.print("❌ Failed to find Image Information service UUID: ");
        Serial.println(imageServiceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found Image Information service");

    // Lấy đặc tính Ball Image
    pRemoteCharImage = pImageService->getCharacteristic(charUUIDImage);
    if (pRemoteCharImage == nullptr)
    {
        Serial.print("❌ Failed to find Image characteristic UUID: ");
        Serial.println(charUUIDImage.toString().c_str());
        pClient->disconnect();
        return false;
    }
    else
    {
        Serial.println(" - Found Image characteristic");
        if (pRemoteCharImage->canNotify())
        {
            pRemoteCharImage->registerForNotify(notifyCallback);
            Serial.println("📡 Registered for Notify on Image");
        }
    }

    delay(1000);
    return true;
}

// Callback khi quet thiet bi
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.print("📡 BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        // if (advertisedDevice.getAddress().equals(BLEAddress("E4:5F:01:D7:A7:F3")))
        if (advertisedDevice.getAddress().equals(BLEAddress("D8:3A:DD:52:2D:BC")))
        {
            Serial.println("✅ Found MyGMQ device!");
            advertisedDevice.getScan()->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());
            doConnect = true;
        }
    }
};

void setup()
{
    Serial.begin(115200);
    delay(1000);
    Serial.println("🔵 Starting Arduino BLE Client for MyGMQ...");

    BLEDevice::init("");

    clearBondingInfo();
    Serial.println("🧹 Cleared previous BLE bonds");

    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(30);
}

void loop()
{
    if (doConnect)
    {
        if (connectToServer(*pServerAddress))
        {
            Serial.println("✅ Connected to MyGMQ!");
        }
        else
        {
            Serial.println("❌ Failed to connect to MyGMQ!");
        }
        doConnect = false;
    }

    if (connected && pClient && pClient->isConnected())
    {
        // Serial.println("📡 Reading data from MyGMQ...");
        // readCharacteristicData();
    }
    else if (!connected)
    {
        Serial.println("🔄 Connection lost. Scanning again...");
        BLEScan *pBLEScan = BLEDevice::getScan();
        pBLEScan->start(30);
    }

    delay(5000);
}