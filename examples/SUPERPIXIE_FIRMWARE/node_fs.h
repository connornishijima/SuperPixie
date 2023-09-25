storage STORAGE;

// #############################################################################################
// Initialize the storage to default values on boot
void init_storage() {
  //debugln("init_storage()");
  memcpy(&STORAGE, &STORAGE_DEFAULTS, sizeof(storage));
}
// #############################################################################################

// Load configuration from LittleFS
void load_storage() {
  // Open "/STORAGE.BIN" file for reading
  File file = LittleFS.open("/STORAGE.BIN", "r");
  if (!file) {
    //debugln("Failed to open /STORAGE.BIN for reading");
    //debugln("Formatting LittleFS just in case it's corrupted...");
    LittleFS.format();
    return;
  } else {
    //debugln("File opened successfully!");
  }

  // Read STORAGE struct byte-by-byte
  byte *ptr = (byte *)&STORAGE;
  for (size_t i = 0; i < sizeof(STORAGE); i++) {
    *ptr++ = file.read();
  }

  // Close the file
  file.close();
}

// Save storage to LittleFS
void save_storage() {
  // Open "/STORAGE.BIN" file for writing
  File file = LittleFS.open("/STORAGE.BIN", "w+");
  if (!file) {
    //debugln("Failed to open /STORAGE.BIN for writing");
    return;
  } else {
    //debugln("/STORAGE.BIN opened for writing");

    // Write STORAGE struct byte-by-byte
    uint8_t *ptr = (uint8_t *)&STORAGE;
    for (size_t i = 0; i < sizeof(STORAGE); i++) {
      file.write(*ptr++);
    }

    // Close the file
    file.close();
  }
}

// Initialize LittleFS
void init_fs() {
  //debugln("------------$");
  //debugln(STORAGE.TOUCH_THRESHOLD);
  //debugln(STORAGE.TOUCH_HIGH_LEVEL);
  //debugln(STORAGE.TOUCH_LOW_LEVEL);
  //debugln("------------$");

  //debug("INIT FILESYSTEM: ");

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    //debugln("An error occurred while mounting LittleFS");
    //debugln("Formatting LittleFS just in case it's corrupted...");
    LittleFS.format();
    return;
  } else {
    //debugln("PASS");

    load_storage();

    //debugln("------------$");
    //debugln(STORAGE.TOUCH_THRESHOLD);
    //debugln(STORAGE.TOUCH_HIGH_LEVEL);
    //debugln(STORAGE.TOUCH_LOW_LEVEL);
    //debugln("------------$");
  }
}