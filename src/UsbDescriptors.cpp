#include <cstdint>
#include <cstring>

#include "etl/string_view.h"
#include "etl/array.h"

#include "pico/unique_id.h"
#include "tusb.h"

// WCID implementation inspired by https://github.com/pbatard/libwdi/wiki/WCID-Devices 

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+
static constexpr std::uint16_t VID {0xF055}; // obsolete USB IF VID
static constexpr std::uint16_t PID {0xB195}; // some random number
static constexpr std::uint8_t WCID_VENDOR_ID {0x4A};

static constexpr tusb_desc_device_t DEVICE_DESCRIPTOR
{
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = TUSB_CLASS_VENDOR_SPECIFIC,
    .bDeviceSubClass = 0xFF,
    .bDeviceProtocol = 0xFF,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = VID,
    .idProduct = PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};


//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
static constexpr std::uint8_t LANGUAGE_STRING_DESCRIPTOR_INDEX {0};
static constexpr std::uint8_t VENDOR_INTERFACE_STRING_DESCRIPTOR_INDEX {DEVICE_DESCRIPTOR.iSerialNumber + 1};


static constexpr unsigned CONFIG_TOTAL_LEN {TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN};

static constexpr std::uint8_t VENDOR_ENDPOINT {0x01};

static constexpr std::uint8_t const DESCRIPTOR_CONFIGURATION[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, 1, LANGUAGE_STRING_DESCRIPTOR_INDEX, CONFIG_TOTAL_LEN, 0x00, 500),

    // Interface number, string index, EP Out & IN address, EP size
    TUD_VENDOR_DESCRIPTOR(0, VENDOR_INTERFACE_STRING_DESCRIPTOR_INDEX, VENDOR_ENDPOINT, 0x80 | VENDOR_ENDPOINT, CFG_TUD_ENDPOINT0_SIZE)
};


//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
static constexpr std::uint8_t MICROSOFT_OS_STRING_DESCRIPTOR_INDEX {0xEE};
constexpr char16_t WCID_STRING[]{'M', 'S', 'F', 'T', '1', '0', '0', static_cast<char16_t>(WCID_VENDOR_ID), u'\0'};
constexpr etl::array<etl::pair<std::uint8_t, etl::basic_string_view<char16_t>>, 5> STRING_DESCRIPTORS
{
    etl::pair{LANGUAGE_STRING_DESCRIPTOR_INDEX,         etl::u16string_view{u"\u0409"}},                  // Language ID (US English)
    etl::pair{DEVICE_DESCRIPTOR.iManufacturer,          etl::u16string_view{u"https://github.com/julr"}}, // Vendor 
    etl::pair{DEVICE_DESCRIPTOR.iProduct,               etl::u16string_view{u"USB IR Receiver"}},         // Product
    etl::pair{VENDOR_INTERFACE_STRING_DESCRIPTOR_INDEX, etl::u16string_view{u"Vendor Interface"}},        // Vendor interface name
    etl::pair{MICROSOFT_OS_STRING_DESCRIPTOR_INDEX,     etl::u16string_view{WCID_STRING}}                 // WCID
};
static char16_t* getDeviceSerialNumber()
{
    static char16_t serialUtf16[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES+1]{u'\0'};

    if(serialUtf16[0] == u'\0') // only read and convert the serial once
    {
        char serialAscii[2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES+1];
        pico_get_unique_board_id_string(serialAscii, sizeof(serialAscii));
        for(std::size_t i = 0; i < sizeof(serialAscii); i++)
        {
            serialUtf16[i] = static_cast<char16_t>(serialAscii[i]);
        }
    }

    return serialUtf16;
}


//--------------------------------------------------------------------+
// Microsoft Compatible ID Feature Descriptor
//--------------------------------------------------------------------+
struct TU_ATTR_PACKED ExtendedCompatIdOSFeatureDescriptor
{
    std::uint32_t dwLength;              ///< The length, in bytes, of the complete extended compat ID descriptor
    std::uint16_t bcdVersion;            ///< The descriptor’s version number, in binary coded decimal (BCD) format
    std::uint16_t wIndex;                ///< An index that identifies the particular OS feature descriptor
    std::uint8_t  bCount;                ///< The number of custom property sections
    std::uint8_t  RESERVED_0[7];         ///< Reserved
    std::uint8_t  bFirstInterfaceNumber; ///< The interface or function number
    std::uint8_t  RESERVED_1;            ///< Reserved for system use. Set this value to 0x01
    char          compatibleID[8];       ///< The function’s compatible ID
    std::uint8_t  subCompatibleID[8];    ///< The function’s subcompatible ID
    std::uint8_t  RESERVED_2[6];         ///< Reserved
};

static constexpr ExtendedCompatIdOSFeatureDescriptor MSFT_COMPATIBLE_ID_FEATURE_DESCRIPTOR
{
    .dwLength = sizeof(ExtendedCompatIdOSFeatureDescriptor),
    .bcdVersion = 0x0100,
    .wIndex = 4,
    .bCount = 1,
    .RESERVED_0{0},
    .bFirstInterfaceNumber = 0,
    .RESERVED_1 = 1,
    .compatibleID{"WINUSB"},
    .subCompatibleID{0},
    .RESERVED_2{0}
};


//--------------------------------------------------------------------+
// Microsoft Extended Properties Feature Descriptor
//--------------------------------------------------------------------+
template<typename Data> 
struct TU_ATTR_PACKED ExtendedPropertiesOSFeatureDescriptor 
{
    std::uint32_t dwLength;              ///< The length, in bytes, of the complete extended properties descriptor
    std::uint16_t bcdVersion;            ///< The descriptor’s version number, in binary coded decimal (BCD) format
    std::uint16_t wIndex;                ///< The index for extended properties OS descriptors
    std::uint16_t wCount;                ///< The number of custom property sections that follow the header section
    Data          data;                  ///< The data that follows (one ore more property sections)
};

struct TU_ATTR_PACKED DeviceInterfaceGUIDPropertySection
{
    std::uint32_t dwSize;                ///< The size of this custom properties section
    std::uint32_t dwPropertyDataType;    ///< Property data format
    std::uint16_t wPropertyNameLength;   ///< Property name length
    char16_t      bPropertyName[20];     ///< The property name
    std::uint32_t dwPropertyDataLength;  ///< Length of the buffer holding the property data
    char16_t      bPropertyData[39];     ///< Property data
};

using DeviceInterfaceGUIDDescriptor = ExtendedPropertiesOSFeatureDescriptor<DeviceInterfaceGUIDPropertySection>;
static constexpr DeviceInterfaceGUIDDescriptor MSFT_EXTENDED_PROPERTIES_FEATURE_DESCRIPTOR
{
    .dwLength = sizeof(DeviceInterfaceGUIDDescriptor),
    .bcdVersion = 0x0100,
    .wIndex = 5,
    .wCount = 1,
    .data
    {
        .dwSize = sizeof(DeviceInterfaceGUIDPropertySection),
        .dwPropertyDataType = 1, //1 = Unicode REG_SZ
        .wPropertyNameLength = 40,
        .bPropertyName{u"DeviceInterfaceGUID"},
        .dwPropertyDataLength = 78,
        .bPropertyData{u"{6E3DAB08-A465-4FA0-BA47-BA191F17F5E8}"}
    }
};


//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
extern "C" std::uint8_t const * tud_descriptor_device_cb(void)
{
    return reinterpret_cast<std::uint8_t*>(const_cast<tusb_desc_device_t*>( &DEVICE_DESCRIPTOR ));
}

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
extern "C" std::uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    static_cast<void>(index); // for multiple configurations, not used here
    return DESCRIPTOR_CONFIGURATION;
}

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
extern "C" std::uint16_t const* tud_descriptor_string_cb(std::uint8_t index, std::uint16_t /* langid */)
{    
    static std::uint16_t descriptorString[32]; // make static, so it will be valid even after the function ended

    const char16_t* sourceString{nullptr};
    std::size_t length; // length in chars (one char = 2 byte)

    // Special case: The Serial number is derived from chip ID and therefore not part of the (compile time created) string table
    if(index == DEVICE_DESCRIPTOR.iSerialNumber)
    {
        sourceString = getDeviceSerialNumber();
        length = 2*PICO_UNIQUE_BOARD_ID_SIZE_BYTES;
    }
    else
    {
        for(const auto& entry : STRING_DESCRIPTORS)
        {
            if(entry.first == index)
            {
                sourceString = entry.second.data();
                length = entry.second.length();
                break;
            }
        }
    }

    // We must copy the string since the first "character" contains the metadata
    if(sourceString != nullptr)
    {
        if(length > 31) length = 31; //cap at 31 chars, we have 32 available -1 for metadata
        // metadata: id + length in bytes
        descriptorString[0] = (TUSB_DESC_STRING << 8) | (2*length + 2);
        memcpy(&descriptorString[1], sourceString, length*2);
        
        return descriptorString;
    }
    return nullptr;
}

// Invoked when a control transfer occurred on an interface of this class
// Driver response accordingly to the request and the transfer stage (setup/data/ack)
// return false to stall control endpoint (e.g unsupported request)
extern "C" bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, const tusb_control_request_t* request)
{ 
    if (stage == CONTROL_STAGE_SETUP)
    {
        if ((request->wIndex == MSFT_COMPATIBLE_ID_FEATURE_DESCRIPTOR.wIndex) && (request->bRequest == WCID_VENDOR_ID)) // Microsoft Compatible ID Feature Descriptor requested
        {
            return tud_control_xfer(rhport, request, const_cast<ExtendedCompatIdOSFeatureDescriptor*>(&MSFT_COMPATIBLE_ID_FEATURE_DESCRIPTOR), static_cast<std::uint16_t>(sizeof(ExtendedCompatIdOSFeatureDescriptor)));
        }
        else if((request->wIndex == MSFT_EXTENDED_PROPERTIES_FEATURE_DESCRIPTOR.wIndex) && (request->bmRequestType == 0xC1)) // Microsoft Extended Properties Feature Descriptor requested
        {
            return tud_control_xfer(rhport, request, const_cast<DeviceInterfaceGUIDDescriptor*>(&MSFT_EXTENDED_PROPERTIES_FEATURE_DESCRIPTOR), static_cast<std::uint16_t>(sizeof(MSFT_EXTENDED_PROPERTIES_FEATURE_DESCRIPTOR)));
        }
        else // stall unknown requests
        {
            return false;
        }
    }
    else // nothing to with DATA & ACK stage
    {
        return true;
    }
}