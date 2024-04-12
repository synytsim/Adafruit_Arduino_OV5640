#include "ov5640.h"

_SCCB16CameraBase::OV5640_ERR _SCCB16CameraBase::_getLastError() {
    return OV5640_ERROR;
}

void _SCCB16CameraBase::_write_register(int reg, int value) {
    byte b[3];
    b[0] = reg >> 8;
    b[1] = reg & 0xFF;
    b[2] = value;

    // Serial.print(b[0], BIN);
    // Serial.print(b[1], BIN);
    // Serial.println(b[2], BIN);

    int numByteSent = 0;
    // this->_i2c_device->begin();
    this->_i2c_device->beginTransmission(this->_i2c_address);
    numByteSent = this->_i2c_device->write(b, 3);
    int result = this->_i2c_device->endTransmission();
    // this->_i2c_device->end();

    if (result != 0) { 
        Serial.printf("write to register 0x%.2X failed, error: %d\r\n", reg, result);
        OV5640_ERROR = I2C;
        return;
    }
    OV5640_ERROR = OK;
    //delay(300);
    //else Serial.printf("write to register 0x%.2X success, wrote %d bytes\r\n", reg, numByteSent);
}

void _SCCB16CameraBase::_write_addr_reg(int reg, int x_value, int y_value) {
    this->_write_register16(reg, x_value);
    this->_write_register16(reg + 2, y_value);
}

void _SCCB16CameraBase::_write_register16(int reg, int value) {
    this->_write_register(reg, value >> 8);
    this->_write_register(reg + 1, value & 0xFF);
}

int _SCCB16CameraBase::_read_register(int reg) {
    byte b[2];
    b[0] = reg >> 8;
    b[1] = reg & 0xFF;

    // this->_i2c_device->begin();
    this->_i2c_device->beginTransmission(this->_i2c_address);
    this->_i2c_device->write(b, 2);
    this->_i2c_device->endTransmission();

    this->_i2c_device->requestFrom(this->_i2c_address, 1);
    //while (this->_i2c_device->available()) {
    return this->_i2c_device->read();
    
    b[0] = this->_i2c_device->read();
    Serial.printf("reading from register 0x%.2X, returned: %d\r\n", reg, b[0]);
    return b[0];
}

int _SCCB16CameraBase::_read_register16(int reg) {
    int high = this->_read_register(reg);
    int low = this->_read_register(reg + 1);

    return (high << 8) | low;
}

void _SCCB16CameraBase::_write_list(const int* reg_list, int reg_list_size) {
    for (int i = 0; i < reg_list_size; i += 2) {
        int reg = reg_list[i];
        int value = reg_list[i + 1];

        if (reg == _REG_DLY) delay(value);
        else this->_write_register(reg, value);
    }
}

void _SCCB16CameraBase::_write_reg_bits(int reg, int mask, bool enable) {
    int val = this->_read_register(reg);

    if (enable) val |= mask;
    else val &= ~mask;

    this->_write_register(reg, val);
}

OV5640::OV5640(TwoWire* i2c_device, int* data_pins, int pixel_clock, int vsync, int href, int shutdown, int reset, int mclk, int mclk_frequency, int i2c_address, int size) {
    this->_i2c_device = i2c_device;
    this->data_pins = data_pins;
    this->clock = pixel_clock;
    this->vsync = vsync;
    this->href = href;
    this->shutdown = shutdown;
    this->reset = reset;
    this->mclk = mclk;
    this->mclk_frequency = mclk_frequency;
    this->_i2c_address = i2c_address;
    this->size = size;
}

volatile uint32_t *vsync_reg, *hsync_reg;
uint32_t vsync_bit, hsync_bit;

void OV5640::init() {

    this->_colorspace = OV5640_COLOR_JPEG;
    this->_flip_x = 0;
    this->_flip_y = 0;
    this->_w = 0;
    this->_h = 0;
    this->_size = 0;
    this->_binning = 0;
    this->_scale = 0;
    this->_ev = 0;
    this->_white_balance = 0;
    this->size = size;

    // this->_i2c_device->begin();
    // this->_i2c_device->setClock(100000);
    // delay(1); 

    if (this->reset > 0) {
        pinMode(this->reset, OUTPUT);
        digitalWrite(this->reset, LOW);
    }

    if (this->shutdown > 0) {
        pinMode(this->shutdown, OUTPUT);
        digitalWrite(this->shutdown, HIGH);
        delay(5);
        digitalWrite(this->shutdown, LOW);
    }

    if (this->reset > 0) {
        delay(1);
        digitalWrite(this->reset, HIGH);
        delay(20);
    }

    // delay(2);
    // digitalWrite(this->reset, LOW);
    // delay(2);

    // digitalWrite(this->reset, HIGH);
    // pinMode(this->reset, INPUT);

    // delay(20);

    Serial.print("CAM CHIP ID: ");

    uint16_t chip_id = this->getChipId();
    Serial.println(chip_id, HEX);

    if (chip_id != 0x5640) {
        Serial.println("WARNING: CHIP ID INVALID!");
    }

    Serial.println("INITIALIZING CAMERA");

    this->_write_list(_sensor_default_regs, int(sizeof(_sensor_default_regs) / sizeof(int)));

    PCC->MR.bit.PCEN = 0;
    //Serial.println(PCC->MR.bit.PCEN);
    //delay(1000);

    PCC->IDR.reg = 0b1111;       // Disable all PCC interrupts
    // PCC->IER.reg = 0b0000;       // Disable all PCC interrupts
    MCLK->APBDMASK.bit.PCC_ = 1;


    // PCC->IDR.bit.OVRE = 0;
    // PCC->IDR.bit.DRDY = 0;
    // PCC->IER.bit.OVRE = 1;
    // PCC->IER.bit.DRDY = 1;

    // Serial.println(PCC->IMR.bit.DRDY, BIN);
    // Serial.println(PCC->IMR.bit.OVRE, BIN);

    pinPeripheral(this->clock, PIO_PCC);
    pinPeripheral(this->vsync, PIO_PCC);
    pinPeripheral(this->href, PIO_PCC);
    pinPeripheral(this->data_pins[0], PIO_PCC);
    pinPeripheral(this->data_pins[1], PIO_PCC);
    pinPeripheral(this->data_pins[2], PIO_PCC);
    pinPeripheral(this->data_pins[3], PIO_PCC);
    pinPeripheral(this->data_pins[4], PIO_PCC);
    pinPeripheral(this->data_pins[5], PIO_PCC);
    pinPeripheral(this->data_pins[6], PIO_PCC);
    pinPeripheral(this->data_pins[7], PIO_PCC);

    //Accumulate 4 bytes into RHR register (two 16-bit pixels)
    PCC->MR.reg = PCC_MR_CID(0x1) |   // Clear on falling DEN1 (VSYNC)
                PCC_MR_ISIZE(0x0) | // Input data bus is 8 bits
                PCC_MR_DSIZE(0x2);  // "4 data" at a time (accumulate in RHR)

    // Accumulate 4 bytes into RHR register (two 16-bit pixels)
    // PCC->MR.reg = PCC_MR_CID(0x1) |   // Clear on falling DEN1 (VSYNC)
    //             PCC_MR_ISIZE(0x0) | // Input data bus is 8 bits
    //             PCC_MR_FRSTS(0x0) |
    //             PCC_MR_HALFS(0x0) |
    //             PCC_MR_ALWYS(0x0) |
    //             PCC_MR_SCALE(0x0) |
    //             PCC_MR_DSIZE(0x2);  // "4 data" at a time (accumulate in RHR)

    //PCC->MR.bit.PCEN = 1;

    // volatile uint32_t *vsync_reg, *hsync_reg;
    // uint32_t vsync_bit, hsync_bit;

    vsync_reg = &PORT->Group[g_APinDescription[this->vsync].ulPort].IN.reg;
    vsync_bit = 1ul << g_APinDescription[this->vsync].ulPin;
    hsync_reg = &PORT->Group[g_APinDescription[this->href].ulPort].IN.reg;
    hsync_bit = 1ul << g_APinDescription[this->href].ulPin;

}

OV5640::~OV5640() {
    // delete chip_id;
}

int OV5640::getChipId(void) {
    return this->_read_register16(_CHIP_ID_HIGH);
}

uint8_t *buf = NULL;

static bool OV5640_POWER_ON = 0;
void OV5640::_powerOn() {
    //digitalWrite(this->shutdown, LOW);
    //delay(1);
    if (!OV5640_POWER_ON) {
        this->_write_register(_SYSTEM_CTROL0, 0x2); // power on

        while (this->_getLastError() == I2C) {
            this->_write_register(_SYSTEM_CTROL0, 0x2);
        }
    }
    //Serial.println("Power On");
    OV5640_POWER_ON = 1;
}

void OV5640::_powerOff() {
    // use shutdown pin here?
    //digitalWrite(this->shutdown, HIGH);
    //delay(5);
    if (OV5640_POWER_ON) {
        this->_write_register(_SYSTEM_CTROL0, 0x42); // power off
        
        while (this->_getLastError() == I2C) {
            this->_write_register(_SYSTEM_CTROL0, 0x42);
        }
    }
    //Serial.println("Power Off");
    OV5640_POWER_ON = 0;
}

// PARALLEL CAPTURE STUFF!!!!
void OV5640::capture(void) {

    Serial.println("Attemping capture...");

    //uint32_t bufSize = capture_buffer_size();
    uint32_t bufSize = _w * _h;

    if (buf != NULL) free(buf);
    buf = NULL;

    //Serial.println("Attempting malloc()");

    buf = (uint8_t*)malloc(bufSize);

    if (buf == NULL) {
        Serial.println("malloc() failed!");
        return;
    }

    Serial.println("malloc() success");

    //Serial.println("taking photo");

    // uint8_t _temp = (this->quality());
    // uint8_t _q = _temp >> 1;
    
    // uint16_t _width = this->_w / (_q > 0 ? 1 : _q);
    // uint16_t _height = this->_h / (_q > 0 ? 1 : _q);

    uint16_t _width = this->_w;
    uint16_t _height = this->_h;

    Serial.printf("Width: %d, Heigth: %d, Total: %d", _width, _height, bufSize);
    Serial.println();

    _width = _width >> 2;
    
    uint32_t *bufPtr = (uint32_t*)buf;

    this->_powerOn();
    delay(300);

    PCC->MR.bit.PCEN = 1;

    //Serial.println("START");

    while(*vsync_reg & vsync_bit)
        ;

    noInterrupts();

    while(!*vsync_reg & vsync_bit)
        ;
    //Serial.println("A");

    for (uint16_t y = 0; y < _height; y++) {
        while (*hsync_reg & hsync_bit)
            ;

        while (!*hsync_reg & hsync_bit)
            ;

        //Serial.println("B");
        

        for (uint16_t x = 0; x < _width; x++) {
            while (!PCC->ISR.bit.DRDY)
                ;
            // if (PCC->ISR.bit.OVRE)
            //     ;
            
            //Serial.println("C");
            
            *bufPtr++ = PCC->RHR.reg;
        }
    }

    PCC->MR.bit.PCEN = 0;

    interrupts();

    //Serial.println("END");

    //delay(100);

    //this->_powerOff();

    // PCC->WPMR.bit.WPEN = 0;
    //PCC->MR.bit.PCEN = 0;
    // PCC->WPMR.bit.WPEN = 1;

    Serial.println("capture done");
}

void OV5640::dump(void) {
    // uint8_t _q = (this->quality()) >> 1;
    
    // uint16_t _width = (this->_w / (_q > 0 ? 1 : _q));
    // uint16_t _height = (this->_h / (_q > 0 ? 1 : _q));

    // Serial.println("Dumping buffer...");
    // bool stop = 0;
    // for (int y = 0; y < this->_h / 3; y++) {
    //     for (int x = 0; x < this->_w / 4; x++) {
    //         Serial.printf("0x%.2X", buf[y * x + x]);
    //         Serial.print(" ");
    //         if (((y * x + x) > 1) && (buf[y * x + x - 1] == 0xFF) && (buf[y * x * x] == 0xD9)) {
    //             Serial.println("OK!!");
    //             stop = 1;
    //             break;
    //         }
    //         delay(1);
    //     }
    //     Serial.println();
    //     if (stop) break;
    //     delay(1);
    // }
    // Serial.printf("Width: %d, Heigth: %d", _width, _height);
    // Serial.println();

    Serial.println("START");

    for (uint32_t i = 0; i < _w * _h; i++) {
        Serial.println(buf[i]);
        if (i > 1 && buf[i - 1] == 0xFF && buf[i] == 0xD9) break;
        //delay(1);
    }
    Serial.println("END");

}

int OV5640::capture_buffer_size(void) {
    if (this->_colorspace == OV5640_COLOR_JPEG) return (this->_w * this->_h) / quality();
    if (this->_colorspace == OV5640_COLOR_GRAYSCALE) return this->_w * this->_h;
    return this->_w * this->_h * 2;
}

int OV5640::get_mclk_frequency(void) const {
    return 0;
}

int OV5640::width(void) const {
    return this->_w;
}

int OV5640::height(void) const {
    return this->_h;
}

int OV5640::colorspace(void) const {
    return this->_colorspace;
}

void OV5640::colorspace(int colorspace) {
    this->_colorspace = colorspace;
    this->_set_size_and_colorspace();
}

void OV5640::_set_image_options(void) {
    int reg20 = 0;
    int reg21 = 0;
    int reg4514 = 0;
    int reg4514_test = 0;

    if (this->_colorspace == OV5640_COLOR_JPEG) reg21 |= 0x20;

    if (this->_binning) {
        reg20 |= 1;
        reg21 |= 1;
        reg4514_test |= 4;
    } else reg20 |= 0x40;

    if (this->_flip_y) {
        reg20 |= 0x06;
        reg4514_test |= 1;
    }

    if (this->_flip_x) {
        reg21 |= 0x06;
        reg4514_test |= 2;
    }

    if (reg4514_test == 0) reg4514 = 0x88;
    else if (reg4514_test == 1) reg4514 = 0x00;
    else if (reg4514_test == 2) reg4514 = 0xBB;
    else if (reg4514_test == 3) reg4514 = 0x00;
    else if (reg4514_test == 4) reg4514 = 0xAA;
    else if (reg4514_test == 5) reg4514 = 0xBB;
    else if (reg4514_test == 6) reg4514 = 0xBB;
    else if (reg4514_test == 7) reg4514 = 0xAA;

    this->_write_register(_TIMING_TC_REG20, reg20);
    this->_write_register(_TIMING_TC_REG21, reg21);
    this->_write_register(0x4514, reg4514);

    if (this->_binning) {
        this->_write_register(0x4520, 0x0B);
        this->_write_register(_X_INCREMENT, 0x31);
        this->_write_register(_Y_INCREMENT, 0x31);
    } else {
        this->_write_register(0x4520, 0x10);
        this->_write_register(_X_INCREMENT, 0x11);
        this->_write_register(_Y_INCREMENT, 0x11);
    }

}

void OV5640::_set_colorspace(void) {
    int _cs = this->_colorspace;
    // int size = _ov5640_color_settings_size[_cs];

    if (_cs == 0) this->_write_list(_sensor_format_rgb565, int(sizeof(_sensor_format_rgb565) / sizeof(int)));
    else if (_cs == 1) this->_write_list(_sensor_format_yuv422, int(sizeof(_sensor_format_yuv422) / sizeof(int)));
    else if (_cs == 2) this->_write_list(_sensor_format_grayscale, int(sizeof(_sensor_format_grayscale) / sizeof(int)));
    else if (_cs == 3) this->_write_list(_sensor_format_jpeg, int(sizeof(_sensor_format_jpeg) / sizeof(int)));
}

int OV5640::getSize(void) const {
    return this->_size;
}

void OV5640::_set_size_and_colorspace(void) {
    int size = this->_size;
    //const int* _this_resolution_info = _resolution_info[size];
    int width = _resolution_info[size][0];
    int height = _resolution_info[size][1];
    int ratio = _resolution_info[size][2];

    this->_w = width;
    this->_h = height;

    
    //const int* _this_ratio_table = _ratio_table[ratio];

    int max_width = _ratio_table[ratio][0];
    int max_height = _ratio_table[ratio][1];
    int start_x = _ratio_table[ratio][2];
    int start_y = _ratio_table[ratio][3];
    int end_x = _ratio_table[ratio][4];
    int end_y = _ratio_table[ratio][5];
    int offset_x = _ratio_table[ratio][6];
    int offset_y = _ratio_table[ratio][7];
    int total_x = _ratio_table[ratio][8];
    int total_y = _ratio_table[ratio][9];

    this->_binning = (width <= max_width / 2) && (height <= max_height / 2);

    this->_scale = !((width == max_width && height == max_height) || (width == max_width / 2 && height == max_height / 2));

    // this->_i2c_device->begin();

    // while(!(_i2c_device->available()))
    //     ;

    this->_write_addr_reg(_X_ADDR_ST_H, start_x, start_y);
    this->_write_addr_reg(_X_ADDR_END_H, end_x, end_y);
    this->_write_addr_reg(_X_OUTPUT_SIZE_H, width, height);

    if (!(this->_binning)) {
        this->_write_addr_reg(_X_TOTAL_SIZE_H, total_x, total_y);
        this->_write_addr_reg(_X_OFFSET_H, offset_x, offset_y);
    } else {
        if (width > 920) this->_write_addr_reg(_X_TOTAL_SIZE_H, total_x - 200, total_y / 2);
        else this->_write_addr_reg(_X_TOTAL_SIZE_H, 2060, total_y / 2);

        this->_write_addr_reg(_X_OFFSET_H, offset_x / 2, offset_y / 2);
    }

    this->_write_reg_bits(_ISP_CONTROL_01, 0x20, this->_scale);

    this->_set_image_options();

    if (this->_colorspace == OV5640_COLOR_JPEG) {
        int sys_mul = 200;
        if (size < OV5640_SIZE_QVGA) sys_mul = 160;
        if (size < OV5640_SIZE_XGA) sys_mul = 180;

        this->_set_pll(0, sys_mul, 4, 2, 0, 2, 1, 4);
    } else this->_set_pll(0, 32, 1, 1, 0, 1, 1, 4);

    this->_set_colorspace();

    //this->_i2c_device->end();

}

void OV5640::_set_pll(
    bool bypass,
    int multiplier,
    int sys_div,
    int pre_div,
    bool root_2x,
    int pclk_root_div,
    bool pclk_manual,
    int pclk_div
) {
    if (multiplier > 252
        || multiplier < 4
        || sys_div > 15
        || pre_div > 8
        || pclk_div > 31
        || pclk_root_div > 3
    ) {
        Serial.println("Invalid arguement to internal function!");
        return;
    }   

    this->_write_register(0x3039, (bypass ? 0x80 : 0));
    this->_write_register(0x3034, 0x1A);
    this->_write_register(0x3035, 1 | ((sys_div & 0xF) << 4));
    this->_write_register(0x3036, multiplier & 0xFF);
    this->_write_register(0x3037, (pre_div & 0xF) | (root_2x ? 0x10 : 0));
    this->_write_register(0x3108, (pclk_root_div & 3) << 4 | 0x06);
    this->_write_register(0x3824, pclk_div & 0x1F);
    this->_write_register(0x460C, 0x22);
    this->_write_register(0x3103, 0x13);
} 

void OV5640::setSize(int size) { 
    this->_size = size;
    this->_set_size_and_colorspace();
}

bool OV5640::flip_x(void) const { return this->_flip_x; }
void OV5640::flip_x(bool value) {
    this->_flip_x = value;
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_set_image_options();
    // this->_i2c_device->end();
}

bool OV5640::flip_y(void) const { return this->_flip_y; }
void OV5640::flip_y(bool value) {
    this->_flip_y = value;
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_set_image_options();
    // this->_i2c_device->end();
}

bool OV5640::test_pattern() const { return this->_test_pattern; }
void OV5640::test_pattern(bool value) {
    this->_test_pattern = value;
    //this->_i2c_device->begin();

    //while(this->_i2c_device->available());

    this->_write_register(_PRE_ISP_TEST_SETTING_1, value << 7);
    
    //this->_i2c_device->end();
}

int OV5640::saturation(void) const { return this->_saturation; }
void OV5640::saturation(int value) {
    if (!(value >= -4 && value <= 4)) {
        Serial.printf("Invalid saturation %d, use value from -4..4 inclusive\r\n", value);
        return;
    }
    //const int* _this_sensor_saturation_levels = _sensor_saturation_levels[value + 4];

    // this->_i2c_device->setClock(100000);
    for (int offset = 0; offset < 11; offset++) {
        int reg_value = _sensor_saturation_levels[value + 4][offset];
        this->_write_register(0x5381 + offset, reg_value);
    }
    // this->_i2c_device->end();
    this->_saturation = value;
}

int OV5640::effect(void) const { return this->_effect; }
void OV5640::effect(int value) {
    int reg_addr[4] = { 0x5580, 0x5583, 0x5584, 0x5003 };
    //const int* _this_sensor_special_effects = _sensor_special_effects[value];
    // this->_i2c_device->setClock(100000);
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    for (int i = 0; i < 4; i++) {
        int reg_value = _sensor_special_effects[value][i];
        this->_write_register(reg_addr[i], reg_value);
    }
    // this->_i2c_device->end();
    this->_effect = value;
}

int OV5640::quality(void) { 
    return _read_register(_COMPRESSION_CTRL07) & 0x3F; 
}

void OV5640::quality(int value) {
    if (!(value >= 2 && value <= 55)) {
        Serial.printf("Invalid quality value %d, use a value from 2..55 inclusive\r\n", value);
        return;
    }

    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_write_register(_COMPRESSION_CTRL07, value & 0x3F);
    // this->_i2c_device->end();
}

void OV5640::_write_group_3_settings(int* settings) {
    this->_write_register(0x3212, 0x3);
    this->_write_list(settings, 4);
    this->_write_register(0x3212, 0x13);
    this->_write_register(0x3212, 0xA3);
}

int OV5640::brightness(void) {
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    int brightness_abs = this->_read_register(0x5587) >> 4;
    int brightness_neg = this->_read_register(0x5588) & 8;
    // this->_i2c_device->end();

    if (brightness_neg) return -brightness_abs;
    return brightness_abs;
}
void OV5640::brightness(int value) {
    if (!(value >= -4 && value <= 4)) {
        Serial.printf("Invalid brightness value %d, use a value from -4..4 inclusive\r\n", value);
        return;
    }
    int temp[4] = { 0x5587, abs(value) << 4, 0x5588, (value < 0 ? 0x9 : 0x1) };

    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_write_group_3_settings(temp);
    // this->_i2c_device->end();
}

int OV5640::contrast(void) {
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    int contrast_abs = this->_read_register(0x5587) >> 4;
    int contrast_neg = this->_read_register(0x5588) & 8;
    // this->_i2c_device->end();

    if (contrast_neg) return -contrast_abs;
    return contrast_abs;
}
void OV5640::contrast(int value) {
    if (!(value >= -3 && value <= 3)) {
        Serial.printf("Invalid contrast value %d, use a value from -3..3 inclusive\r\n", value);
        return;
    }
    //const int* setting = _contrast_settings[value + 3];
    int temp[4] = { 0x5586, _contrast_settings[value + 3][0], 0x5585, _contrast_settings[value + 3][1] };
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_write_group_3_settings(temp);
    // this->_i2c_device->end();
}

int OV5640::exposure_value(void) const { return this->_ev; }
void OV5640::exposure_value(int value) {
    if (!(value >= -3 && value <= 3)) {
        Serial.printf("Invalid exposure value %d, use a value from -3..3 inclusive\r\n", value);
        return;
    }
    // const int* _this_sensor_ev_values = _sensor_ev_levels[value + 3];

    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    for (int offset = 0; offset < 6; offset++) {
        int reg_value = _sensor_ev_levels[value + 3][offset];
        this->_write_register(0x5381 + offset, reg_value);
    }
    // this->_i2c_device->end();
    //this->_ev = value;
}

int OV5640::white_balance(void) const { return this->_white_balance; }
void OV5640::white_balance(int value) {
    if (!(value >= OV5640_WHITE_BALANCE_AUTO  && value <= OV5640_WHITE_BALANCE_INCANDESCENT)) {
        Serial.printf("Invalid white balance value %d, use one of the OV5640_WHITE_BALANCE_* constants", value);
        return;
    }
    //const int* _this_light_mode = _light_modes[value];

    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_write_register(0x3212, 0x3);
    for (int i = 0; i < 7; i++) {
        int reg_addr = _light_registers[i];
        int reg_value = _light_modes[value][i];
        this->_write_register(reg_addr, reg_value);
    }
    this->_write_register(0x3212, 0x13);
    this->_write_register(0x3212, 0xA3);
    // this->_i2c_device->end();
}

bool OV5640::night_mode(void) {
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    bool value = this->_read_register(0x3A00) & 0x04; 
    // this->_i2c_device->end();
    return value;
}
void OV5640::night_mode(bool value) {
    //this->_i2c_device->begin(_i2c_bus[0], _i2c_bus[1]);
    // this->_i2c_device->setClock(100000);
    this->_write_reg_bits(0x3A00, 0x04, value);
    // this->_i2c_device->end();
}
