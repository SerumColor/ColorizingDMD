/** @file ZeDMD.h
 *  @brief ZeDMD client library.
 *
 *  Connecting ZeDMD devices.
 */
#pragma once

#define ZEDMD_VERSION_MAJOR 0  // X Digits
#define ZEDMD_VERSION_MINOR 7  // Max 2 Digits
#define ZEDMD_VERSION_PATCH 2  // Max 2 Digits

#define _ZEDMD_STR(x) #x
#define ZEDMD_STR(x) _ZEDMD_STR(x)

#define ZEDMD_VERSION            \
  ZEDMD_STR(ZEDMD_VERSION_MAJOR) \
  "." ZEDMD_STR(ZEDMD_VERSION_MINOR) "." ZEDMD_STR(ZEDMD_VERSION_PATCH)
#define ZEDMD_MINOR_VERSION ZEDMD_STR(ZEDMD_VERSION_MAJOR) "." ZEDMD_STR(ZEDMD_VERSION_MINOR)

#define ZEDMD_MAX_WIDTH 256
#define ZEDMD_MAX_HEIGHT 64
#define ZEDMD_MAX_PALETTE 192

#ifdef _MSC_VER
#define ZEDMDAPI __declspec(dllexport)
#define ZEDMDCALLBACK __stdcall
#else
#define ZEDMDAPI __attribute__((visibility("default")))
#define ZEDMDCALLBACK
#endif

#include <inttypes.h>
#include <stdarg.h>

#include <cstdio>

typedef void(ZEDMDCALLBACK* ZeDMD_LogCallback)(const char* format, va_list args, const void* userData);

class ZeDMDComm;
class ZeDMDWiFi;

class ZEDMDAPI ZeDMD
{
 public:
  ZeDMD();
  ~ZeDMD();

  void SetLogCallback(ZeDMD_LogCallback callback, const void* userData);

  /** @brief Ignore a serial device when searching for ZeDMD
   *
   *  While searching for a ZeDMD any serial ports are tested.
   *  This could cause trouble with other devices attached via
   *  USB or serial ports.
   *  Using this mfunction, a serial device could be excluded
   *  from the scan.
   *  This function could be called multiple times to ignore
   *  multiple devices.
   *  Another option to limit the searching is to use SetDevice().
   *  @see SetDevice()
   *  @see Open()
   *
   *  @param ignore_device the device to ignore
   */
  void IgnoreDevice(const char* const ignore_device);

  /** @brief Use a specific serial device for ZeDMD
   *
   *  Instead of serching through all serial devices for a ZeDMD,
   *  just use this device.
   *  @see Open()
   *
   *  @param device the device
   */
  void SetDevice(const char* const device);

  /** @brief Open the connection to ZeDMD
   *
   *  Open a cennection to ZeDMD. Therefore all serial ports will be
   *  scanned. Use IgnoreDevice() to exclude one or more specific
   *  serial devices during that scan. Use SetDevice() to omit the
   *  the scan and to use a specific seriel device instead.
   *  @see IgnoreDevice()
   *  @see SetDevice()
   */
  bool Open();

  /** @brief Open the connection to ZeDMD
   *
   *  Backward compatibiility version of Open() which additionally
   *  sets the frame size. Use Open() and SetFrameSize() instead.
   *  @see Open()
   *  @see SetFrameSize()
   *
   *  @deprecated
   *
   *  @param width the frame width
   *  @param height the frame height
   */
  bool Open(uint16_t width, uint16_t height);

  /** @brief Open a WiFi connection to ZeDMD
   *
   *  ZeDMD could be connected via WiFi instead of USB.
   *  The WiFi settings need to be stored in ZeDMD's EEPROM
   *  first using a USB connection.
   *  @see Open()
   *  @see SetWiFiSSID()
   *  @see SetWiFiPassword()
   *  @see SetWiFiPort()
   *  @see SaveSettings()
   *
   *  @param ip the IPv4 address of the ZeDMD device
   *  @param port the port
   */
  bool OpenWiFi(const char* ip, int port);

  /** @brief Close connection to ZeDMD
   *
   *  Close connection to ZeDMD.
   */
  void Close();

  /** @brief Reset ZeDMD
   *
   *  Reset ZeDMD.
   */
  void Reset();

  /** @brief Set the frame size
   *
   *  Set the frame size of the content that will be displayed
   *  next on the ZeDMD device. Depending on the settings and
   *  the physical dimensions of the LED panels, the content
   *  will by centered and scaled correctly.
   *  @see EnablePreDownscaling()
   *  @see EnablePreUpscaling()
   *  @see EnableDownscaling()
   *  @see EnableUpscaling()
   *
   *  @param width the frame width
   *  @param height the frame height
   */
  void SetFrameSize(uint16_t width, uint16_t height);

  /** @brief Get the physical panel width
   *
   *  Get the width of the physical dimensions of the LED panels.
   *
   *  @return width
   */
  uint16_t const GetWidth();

  /** @brief Get the physical panel height
   *
   *  Get the height of the physical dimensions of the LED panels.
   *
   *  @return height
   */
  uint16_t const GetHeight();

  /** @brief Does ZeDMD run on an ESP32 S3?
   *
   *  On an ESP32 S3 a native USB connection is used to increase
   *  the bandwidth. Furthermore double buffering is active.
   *
   *  @return true if an ESP32 S3 is used.
   */
  bool const IsS3();

  /** @brief Set the palette
   *
   *  Set the color palette to use to render gray scaled content.
   *  This library stores and tracks changes to 4, 16 and 64
   *  color palettes individually.
   *  @see RenderGray2()
   *  @see RenderGray4()
   *
   *  @param pPalette the palette as RGB array
   *  @param numColors 4, 16, or 64 colors palette
   */
  void SetPalette(uint8_t* pPalette, uint8_t numColors);

  /** @brief Set the 4 color palette
   *
   *  Backward compatibility version of SetPalette() to directly
   *  set the 4 color palette.
   *  @see RenderGray4()
   *
   *  @param pPalette the palette as RGB array
   *  @param numColors 4, 16, or 64 colors palette
   */
  void SetPalette(uint8_t* pPalette);

  /** @brief Set the default palette
   *
   *  Use a default palette of shades of orange to render gray
   *  scaled content.
   *  @see RenderGray2()
   *  @see RenderGray4()
   *
   *  @param bitDepth the bit depth, 2 means 4 colors, 4 means 16 colors
   */
  void SetDefaultPalette(uint8_t bitDepth);

  /** @brief Get the default palette
   *
   *  Get the values of a default palette of shades of orange.
   *
   *  @param bitDepth the bit depth, 2 means 4 colors, 4 means 16 colors
   *  @return RGB array
   */
  uint8_t* GetDefaultPalette(uint8_t bitDepth);

  /** @brief Test the panels attached to ZeDMD
   *
   *  Renders a sequence of full red, full green and full blue frames.
   */
  void LedTest();

  /** @brief Enable debug mode
   *
   *  ZeDMD will display various debug information as overlay to
   *  the displayed frame.
   *  @see https://github.com/PPUC/ZeDMD
   */
  void EnableDebug();

  /** @brief Disable debug mode
   *
   *  @see EnableDebug()
   */
  void DisableDebug();

  /** @brief Set the RGB order
   *
   *  ZeDMD supports different LED panels.
   *  Depending on the panel, the RGB order needs to be adjusted.
   *  @see https://github.com/PPUC/ZeDMD
   *
   *  @param rgbOrder a value between 0 and 5
   */
  void SetRGBOrder(uint8_t rgbOrder);

  /** @brief Set the brightness
   *
   *  Set the brightness of the LED panels.
   *  @see https://github.com/PPUC/ZeDMD
   *
   *  @param brightness a value between 0 and 15
   */
  void SetBrightness(uint8_t brightness);

  /** @brief Set the WiFi SSID
   *
   *  Set the WiFi SSID ZeDMD should connect with.
   *  @see https://github.com/PPUC/ZeDMD
   *
   *  @param brightness a value between 0 and 15
   */
  void SetWiFiSSID(const char* const ssid);

  /** @brief Set the WiFi Password
   *
   *  Set the WiFi Password ZeDMD should use to connect.
   *  @see https://github.com/PPUC/ZeDMD
   *
   *  @param password the password
   */
  void SetWiFiPassword(const char* const password);

  /** @brief Set the WiFi Port
   *
   *  Set the Port ZeDMD should listen at over WiFi.
   *  @see https://github.com/PPUC/ZeDMD
   *
   *  @param port the port
   */
  void SetWiFiPort(int port);

  /** @brief Save the current setting
   *
   *  Saves all current setting within ZeDMD's EEPROM to be used
   *  as defualt at its next start.
   *  @see https://github.com/PPUC/ZeDMD
   *  @see SetRGBOrder()
   *  @see SetBrightness()
   *  @see SetWiFiSSID()
   *  @see SetWiFiPassword()
   *  @see SetWiFiPort()
   *
   *  @param brightness a value between 0 and 15
   */
  void SaveSettings();

  /** @brief Enable downscaling on the client side
   *
   *  If enabled, the content will centered and scaled down to
   *  fit into the physical dimensions of the ZeDMD panels,
   *  before the content gets send to ZeDMD, if required.
   */
  void EnablePreDownscaling();

  /** @brief Disable downscaling on the client side
   *
   *  @see EnablePreDownscaling()
   */
  void DisablePreDownscaling();

  /** @brief Enable upscaling on the client side
   *
   *  If enabled, the content will centered and scaled up to
   *  fit into the physical dimensions of the ZeDMD panels,
   *  before the content gets send to ZeDMD, if required.
   */
  void EnablePreUpscaling();

  /** @brief Disable downscaling on the client side
   *
   *  @see EnablePreUpscaling()
   */
  void DisablePreUpscaling();

  /** @brief Enable upscaling on ZeDMD itself
   *
   *  If enabled and required, the content will centered and scaled
   *  up to fit into the physical dimensions of the ZeDMD panels
   *  by ZeDMD itself. Compared to EnablePreUpscaling(), less data
   *  needs to be send to ZeDMD and this might resut in a higher
   *  frame rate.
   *  But this optimized variant won't work with zone streaming modes.
   *  @see EnforceStreaming()
   *  @see DisableRGB24Streaming()
   *  @see RenderRgb24EncodedAs565()
   */
  void EnableUpscaling();

  /** @brief Disable upscaling on ZeDMD itself
   *
   *  @see EnableUpscaling()
   */
  void DisableUpscaling();

  /** @brief Enforce zone streaming
   *
   *  ZeDMD has two different render modes. One renders entire frames.
   *  This is the classic way and works pretty well.
   *  The other one is "zone streaming" which will be enable by this
   *  function. Zone streaming divides a frame into rectangular zones
   *  and only updates zones that have changes compared to the previous
   *  frame. This method results in less data that needs to be transfered
   *  and in smoother animations. But it takes a bit longer if the entire
   *  frame changes. Zone streaming is the default for RenderRGB24() and
   *  RenderRgb24EncodedAs565(). All other modes use the classic way by
   *  default unless EnforceStreaming() is called.
   *  @see RenderGray2()
   *  @see RenderGray4()
   *  @see RenderColoredGray6()
   *  @see RenderRgb24()
   *  @see DisableRGB24Streaming()
   */
  void EnforceStreaming();

  /** @brief Disable zone streaming for RGB24
   *
   *  By default, "zone streaming" is used for RenderRgb24(). That could be
   *  turned off using this function.
   *  @see EnforceStreaming()
   */
  void DisableRGB24Streaming();

  /** @brief Clear the screen
   *
   *  Turn off all pixels of ZeDMD, so a blank black screen will be shown.
   */
  void ClearScreen();

  /** @brief Render a 2 bit gray scaled frame
   *
   *  Renders a 2 bit gray scaled (indexed) frame using 4 colors.
   *  The colors will be used from the current palette set via SetPalette().
   *  @see SetPalette()
   *
   *  @param frame the indexed frame
   */
  void RenderGray2(uint8_t* frame);

  /** @brief Render a 4 bit gray scaled frame
   *
   *  Renders a 4 bit gray scaled (indexed) frame using 16 colors.
   *  The colors will be used from the current palette set via SetPalette().
   *  @see SetPalette()
   *
   *  @param frame the indexed frame
   */
  void RenderGray4(uint8_t* frame);

  /** @brief Render a 6 bit frame
   *
   *  Renders a 6 bit(indexed) frame using 64 colors.
   *  The colors will be used from the current palette set via SetPalette().
   *  ZeDMD is able to rotate parts of the palette natively as defined by the
   *  Serum format until the next frame is received.
   *  @see SetPalette()
   *
   *  @param frame the indexed frame
   *  @param rotations optional rotation command array according to the Serum
   * format
   */
  void RenderColoredGray6(uint8_t* frame, uint8_t* rotations);

  /** @brief Render a 6 bit frame
   *
   *  Renders a 6 bit(indexed) frame using 64 colors.
   *  In oposite to standrad RenderColoredGray6(), the colors will not be used
   *  from the current palette set via SetPalette() but form the one provided
   *  to this function.
   *  ZeDMD is able to rotate parts of the palette natively as defined by the
   *  Serum format until the next frame is received.
   *
   *  @param frame the indexed frame
   *  @param palette the colors to use
   *  @param rotations optional rotation command array according to the Serum
   * format
   */
  void RenderColoredGray6(uint8_t* frame, uint8_t* palette, uint8_t* rotations);

  /** @brief Render a RGB24 frame
   *
   *  Renders a true color RGB frame. By default the zone streaming mode is
   *  used. The encoding is RGB888.
   *  @see DisableRGB24Streaming()
   *
   *  @param frame the RGB frame
   */
  void RenderRgb24(uint8_t* frame);

  /** @brief Render a RGB24 frame
   *
   *  Renders a true color RGB frame. Only zone streaming mode is supported.
   *  The encoding is RGB565.
   *
   *  @param frame the RGB frame
   */
  void RenderRgb24EncodedAs565(uint8_t* frame);

  /** @brief Render a RGB24 frame
   *
   *  Renders a true color RGB frame. Only zone streaming mode is supported.
   *  The encoding is RGB16. In fact, RGB16 is just another name for RGB565.
   *
   *  @param frame the RGB frame
   */
  void RenderRgb24EncodedAs16(uint8_t* frame) { RenderRgb24EncodedAs565(frame); }

  /** @brief Render a RGB565 frame
   *
   *  Renders a true color RGB565 frame. Only zone streaming mode is supported.
   *
   *  @param frame the RGB565 frame
   */
  void RenderRgb565(uint16_t* frame);

  /** @brief Render a RGB16 frame
   *
   *  Renders a true color RGB16 frame. Only zone streaming mode is supported.
   *  In fact, RGB16 is just another name for RGB565.
   *
   *  @param frame the RGB16 frame
   */
  void RenderRgb16(uint16_t* frame) { RenderRgb565(frame); }

 private:
  bool UpdateFrameBuffer8(uint8_t* pFrame);
  bool UpdateFrameBuffer24(uint8_t* pFrame);
  bool UpdateFrameBuffer565(uint16_t* pFrame);
  uint8_t GetScaleMode(uint16_t frameWidth, uint16_t frameHeight, uint16_t* pWidth, uint16_t* pHeight,
                       uint8_t* pXOffset, uint8_t* pYOffset);
  int Scale(uint8_t* pScaledFrame, uint8_t* pFrame, uint8_t bytes, uint16_t* width, uint16_t* height);
  int Scale16(uint8_t* pScaledFrame, uint16_t* pFrame, uint16_t* width, uint16_t* height, bool bigEndian);

  ZeDMDComm* m_pZeDMDComm;
  ZeDMDWiFi* m_pZeDMDWiFi;

  uint16_t m_romWidth;
  uint16_t m_romHeight;

  bool m_usb = false;
  bool m_wifi = false;
  bool m_hd = false;
  bool m_downscaling = false;
  bool m_upscaling = false;
  bool m_streaming = false;
  bool m_rgb24Streaming = true;
  bool m_paletteChanged = false;

  uint8_t* m_pFrameBuffer;
  uint8_t* m_pScaledFrameBuffer;
  uint8_t* m_pCommandBuffer;
  uint8_t* m_pPlanes;
  uint8_t* m_pRgb565Buffer;

  uint8_t m_palette4[4 * 3] = {0};
  uint8_t m_palette16[16 * 3] = {0};
  uint8_t m_palette64[64 * 3] = {0};
  uint8_t m_DmdDefaultPalette2Bit[12] = {0, 0, 0, 144, 34, 0, 192, 76, 0, 255, 127, 0};
  uint8_t m_DmdDefaultPalette4Bit[48] = {0,  0,   0,   51, 25,  0,   64, 32,  0,   77, 38,  0,   89, 44,  0,   102,
                                         51, 0,   115, 57, 0,   128, 64, 0,   140, 70, 0,   153, 76, 0,   166, 83,
                                         0,  179, 89,  0,  191, 95,  0,  204, 102, 0,  230, 114, 0,  255, 127, 0};
};

#ifdef __cplusplus
extern "C"
{
#endif

  extern ZEDMDAPI ZeDMD* ZeDMD_GetInstance();
  extern ZEDMDAPI void ZeDMD_IgnoreDevice(ZeDMD* pZeDMD, const char* const ignore_device);
  extern ZEDMDAPI void ZeDMD_SetDevice(ZeDMD* pZeDMD, const char* const device);
  extern ZEDMDAPI bool ZeDMD_Open(ZeDMD* pZeDMD);
  extern ZEDMDAPI bool ZeDMD_OpenWiFi(ZeDMD* pZeDMD, const char* ip, int port);
  extern ZEDMDAPI void ZeDMD_Close(ZeDMD* pZeDMD);

  extern ZEDMDAPI void ZeDMD_SetFrameSize(ZeDMD* pZeDMD, uint16_t width, uint16_t height);
  extern ZEDMDAPI void ZeDMD_SetPalette(ZeDMD* pZeDMD, uint8_t* pPalette, uint8_t numColors);
  extern ZEDMDAPI void ZeDMD_SetDefaultPalette(ZeDMD* pZeDMD, uint8_t bitDepth);
  extern ZEDMDAPI uint8_t* ZeDMD_GetDefaultPalette(ZeDMD* pZeDMD, uint8_t bitDepth);
  extern ZEDMDAPI void ZeDMD_LedTest(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_EnableDebug(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_DisableDebug(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_SetRGBOrder(ZeDMD* pZeDMD, uint8_t rgbOrder);
  extern ZEDMDAPI void ZeDMD_SetBrightness(ZeDMD* pZeDMD, uint8_t brightness);
  extern ZEDMDAPI void ZeDMD_SaveSettings(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_EnablePreDownscaling(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_DisablePreDownscaling(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_EnablePreUpscaling(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_DisablePreUpscaling(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_EnableUpscaling(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_DisableUpscaling(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_SetWiFiSSID(ZeDMD* pZeDMD, const char* const ssid);
  extern ZEDMDAPI void ZeDMD_SetWiFiPassword(ZeDMD* pZeDMD, const char* const password);
  extern ZEDMDAPI void ZeDMD_SetWiFiPort(ZeDMD* pZeDMD, int port);
  extern ZEDMDAPI void ZeDMD_EnforceStreaming(ZeDMD* pZeDMD);

  extern ZEDMDAPI void ZeDMD_ClearScreen(ZeDMD* pZeDMD);
  extern ZEDMDAPI void ZeDMD_RenderGray2(ZeDMD* pZeDMD, uint8_t* frame);
  extern ZEDMDAPI void ZeDMD_RenderGray4(ZeDMD* pZeDMD, uint8_t* frame);
  extern ZEDMDAPI void ZeDMD_RenderColoredGray6(ZeDMD* pZeDMD, uint8_t* frame, uint8_t* rotations);
  extern ZEDMDAPI void ZeDMD_RenderRgb24(ZeDMD* pZeDMD, uint8_t* frame);
  extern ZEDMDAPI void ZeDMD_RenderRgb24EncodedAs565(ZeDMD* pZeDMD, uint8_t* frame);

#ifdef __cplusplus
}
#endif
