#include "image.h"
#include "byte_stream.h"
#include "file_system.h"
#include "log.h"
#include "path.h"
#include "scope_guard.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "string_util.h"
Log_SetChannel(Image);

using namespace Common;

#if 0
static bool PNGBufferLoader(RGBA8Image* image, const void* buffer, size_t buffer_size);
static bool PNGBufferSaver(const RGBA8Image& image, std::vector<u8>* buffer, int quality);
static bool PNGFileLoader(RGBA8Image* image, const char* filename, std::FILE* fp);
static bool PNGFileSaver(const RGBA8Image& image, const char* filename, std::FILE* fp, int quality);

static bool JPEGBufferLoader(RGBA8Image* image, const void* buffer, size_t buffer_size);
static bool JPEGBufferSaver(const RGBA8Image& image, std::vector<u8>* buffer, int quality);
static bool JPEGFileLoader(RGBA8Image* image, const char* filename, std::FILE* fp);
static bool JPEGFileSaver(const RGBA8Image& image, const char* filename, std::FILE* fp, int quality);
#endif

static bool STBBufferLoader(RGBA8Image* image, const void* buffer, size_t buffer_size);
static bool STBFileLoader(RGBA8Image* image, const char* filename, std::FILE* fp);
static bool STBBufferSaverPNG(const RGBA8Image& image, std::vector<u8>* buffer, int quality);
static bool STBBufferSaverJPEG(const RGBA8Image& image, std::vector<u8>* buffer, int quality);
static bool STBFileSaverPNG(const RGBA8Image& image, const char* filename, std::FILE* fp, int quality);
static bool STBFileSaverJPEG(const RGBA8Image& image, const char* filename, std::FILE* fp, int quality);

struct FormatHandler
{
  const char* extension;
  bool (*buffer_loader)(RGBA8Image*, const void*, size_t);
  bool (*buffer_saver)(const RGBA8Image&, std::vector<u8>*, int);
  bool (*file_loader)(RGBA8Image*, const char*, std::FILE*);
  bool (*file_saver)(const RGBA8Image&, const char*, std::FILE*, int);
};

static constexpr FormatHandler s_format_handlers[] = {
#if 0
  {"png", PNGBufferLoader, PNGBufferSaver, PNGFileLoader, PNGFileSaver},
  {"jpg", JPEGBufferLoader, JPEGBufferSaver, JPEGFileLoader, JPEGFileSaver},
  {"jpeg", JPEGBufferLoader, JPEGBufferSaver, JPEGFileLoader, JPEGFileSaver},
#else
  {"png", STBBufferLoader, STBBufferSaverPNG, STBFileLoader, STBFileSaverPNG},
  {"jpg", STBBufferLoader, STBBufferSaverJPEG, STBFileLoader, STBFileSaverJPEG},
  {"jpeg", STBBufferLoader, STBBufferSaverJPEG, STBFileLoader, STBFileSaverJPEG},
#endif
};

static const FormatHandler* GetFormatHandler(const std::string_view& extension)
{
  if (extension == "png")
  {
    return &s_format_handlers[0];
  }
  else if (extension == "jpg")
  {
    return &s_format_handlers[1];
  }
  else if (extension == "jpeg")
  {
    return &s_format_handlers[2];
  }
  /*for (const FormatHandler& handler : s_format_handlers)
  {
    if (StringUtil::Strncasecmp(extension.data(), handler.extension, extension.size()))
      return &handler;
  }*/

  return nullptr;
}

RGBA8Image::RGBA8Image() = default;

RGBA8Image::RGBA8Image(const RGBA8Image& copy) : Image(copy) {}

RGBA8Image::RGBA8Image(u32 width, u32 height, const u32* pixels) : Image(width, height, pixels) {}

RGBA8Image::RGBA8Image(RGBA8Image&& move) : Image(move) {}

RGBA8Image& RGBA8Image::operator=(const RGBA8Image& copy)
{
  Image<u32>::operator=(copy);
  return *this;
}

RGBA8Image& RGBA8Image::operator=(RGBA8Image&& move)
{
  Image<u32>::operator=(move);
  return *this;
}

bool RGBA8Image::LoadFromFile(const char* filename)
{
  auto fp = FileSystem::OpenManagedCFile(filename, "rb");
  if (!fp)
    return false;

  return LoadFromFile(filename, fp.get());
}

bool RGBA8Image::SaveToFile(const char* filename, int quality) const
{
  auto fp = FileSystem::OpenManagedCFile(filename, "wb");
  if (!fp)
    return false;

  if (SaveToFile(filename, fp.get(), quality))
    return true;

  // save failed
  fp.reset();
  FileSystem::DeleteFile(filename);
  return false;
}

bool RGBA8Image::LoadFromFile(const char* filename, std::FILE* fp)
{
  const std::string_view extension(Path::GetExtension(filename));
  const FormatHandler* handler = GetFormatHandler(extension);
  if (!handler || !handler->file_loader)
  {
    Log_ErrorPrintf("Unknown extension '%.*s'", static_cast<int>(extension.size()), extension.data());
    return false;
  }

  return handler->file_loader(this, filename, fp);
}

bool RGBA8Image::LoadFromBuffer(const char* filename, const void* buffer, size_t buffer_size)
{
  const std::string_view extension(Path::GetExtension(filename));
  const FormatHandler* handler = GetFormatHandler(extension);
  if (!handler || !handler->buffer_loader)
  {
    Log_ErrorPrintf("Unknown extension '%.*s'", static_cast<int>(extension.size()), extension.data());
    return false;
  }

  return handler->buffer_loader(this, buffer, buffer_size);
}

bool RGBA8Image::SaveToFile(const char* filename, std::FILE* fp, int quality) const
{
  const std::string_view extension(Path::GetExtension(filename));
  const FormatHandler* handler = GetFormatHandler(extension);
  if (!handler || !handler->file_saver)
  {
    Log_ErrorPrintf("Unknown extension '%.*s'", static_cast<int>(extension.size()), extension.data());
    return false;
  }

  if (!handler->file_saver(*this, filename, fp, quality))
    return false;

  return (std::fflush(fp) == 0);
}

std::optional<std::vector<u8>> RGBA8Image::SaveToBuffer(const char* filename, int quality) const
{
  std::optional<std::vector<u8>> ret;

  const std::string_view extension(Path::GetExtension(filename));
  const FormatHandler* handler = GetFormatHandler(extension);
  if (!handler || !handler->file_saver)
  {
    Log_ErrorPrintf("Unknown extension '%.*s'", static_cast<int>(extension.size()), extension.data());
    return ret;
  }

  ret = std::vector<u8>();
  if (!handler->buffer_saver(*this, &ret.value(), quality))
    ret.reset();

  return ret;
}

bool STBBufferLoader(RGBA8Image* image, const void* buffer, size_t buffer_size)
{
  int width, height, file_channels;
  u8* pixel_data = stbi_load_from_memory(static_cast<const stbi_uc*>(buffer), static_cast<int>(buffer_size), &width,
                                         &height, &file_channels, 4);
  if (!pixel_data)
  {
    const char* error_reason = stbi_failure_reason();
    Log_ErrorPrintf("Failed to load image from memory: %s", error_reason ? error_reason : "unknown error");
    return false;
  }

  image->SetPixels(static_cast<u32>(width), static_cast<u32>(height), reinterpret_cast<const u32*>(pixel_data));
  stbi_image_free(pixel_data);
  return true;
}

bool STBFileLoader(RGBA8Image* image, const char* filename, std::FILE* fp)
{
  int width, height, file_channels;
  u8* pixel_data = stbi_load_from_file(fp, &width, &height, &file_channels, 4);
  if (!pixel_data)
  {
    const char* error_reason = stbi_failure_reason();
    Log_ErrorPrintf("Failed to load image from memory: %s", error_reason ? error_reason : "unknown error");
    return false;
  }

  image->SetPixels(static_cast<u32>(width), static_cast<u32>(height), reinterpret_cast<const u32*>(pixel_data));
  stbi_image_free(pixel_data);
  return true;
}

bool STBBufferSaverPNG(const RGBA8Image& image, std::vector<u8>* buffer, int quality)
{
  const auto write_func = [](void* context, void* data, int size) {
    std::vector<u8>* buffer = reinterpret_cast<std::vector<u8>*>(data);
    const u32 len = static_cast<u32>(size);
    buffer->resize(buffer->size() + len);
    std::memcpy(buffer->data(), data, len);
  };

  return (stbi_write_png_to_func(write_func, buffer, image.GetWidth(), image.GetHeight(), 4, image.GetPixels(),
                                 image.GetByteStride()));
}

bool STBBufferSaverJPEG(const RGBA8Image& image, std::vector<u8>* buffer, int quality)
{
  const auto write_func = [](void* context, void* data, int size) {
    std::vector<u8>* buffer = reinterpret_cast<std::vector<u8>*>(data);
    const u32 len = static_cast<u32>(size);
    buffer->resize(buffer->size() + len);
    std::memcpy(buffer->data(), data, len);
  };

  return (stbi_write_jpg_to_func(write_func, buffer, image.GetWidth(), image.GetHeight(), 4, image.GetPixels(),
                                 quality));
}

bool STBFileSaverPNG(const RGBA8Image& image, const char* filename, std::FILE* fp, int quality)
{
  const auto write_func = [](void* context, void* data, int size) {
    std::fwrite(data, 1, size, static_cast<std::FILE*>(context));
  };

  return (stbi_write_png_to_func(write_func, fp, image.GetWidth(), image.GetHeight(), 4, image.GetPixels(),
                                 image.GetByteStride()));
}

bool STBFileSaverJPEG(const RGBA8Image& image, const char* filename, std::FILE* fp, int quality)
{
  const auto write_func = [](void* context, void* data, int size) {
    std::fwrite(data, 1, size, static_cast<std::FILE*>(context));
  };

  return (stbi_write_jpg_to_func(write_func, fp, image.GetWidth(), image.GetHeight(), 4, image.GetPixels(), quality));
}