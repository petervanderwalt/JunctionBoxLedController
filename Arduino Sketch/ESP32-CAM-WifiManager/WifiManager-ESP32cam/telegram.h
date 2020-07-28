bool isMoreDataAvailable() {
  return (fb_length - currentByte);
}

uint8_t photoNextByte() {
  currentByte++;
  return (fb_buffer[currentByte - 1]);
}

void take_send_photo(String chat_id)
{

  // restore to high resolution
  camera_config_t config;
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;  // Trying to reduce memory use
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  currentByte = 0;
  fb_length = fb->len;
  fb_buffer = fb->buf;
  bot.sendPhotoByBinary(chat_id, "image/jpeg", fb->len, isMoreDataAvailable, photoNextByte, nullptr, nullptr);
  esp_camera_fb_return(fb);
  fb_length = NULL;
  fb_buffer = NULL;

  // restore to low resolution
  config.frame_size = FRAMESIZE_QVGA;
}
