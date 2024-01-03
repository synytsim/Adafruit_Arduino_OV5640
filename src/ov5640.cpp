_RegBits::_RegBits(int reg, int shift, int mask) {
    this->reg = reg;
    this->shift = shift;
    this->mask = mask;
}

int _RegBits::__get__(OV5640* obj) const {
    int reg_value = obj->_read_register(this->reg);
    return (reg_value >> this->shift) & this->mask;
}

void _RegBits::__set__(OV5640* obj, int value) {
    if (value & ~this->mask) {
        Serial.printf("Value 0x%02x does not fit in mask 0x%02x\r\n", value, this->mask);
        return;
    }

    int reg_value = obj->_read_register(this->reg);
    reg_value &= ~(this->mask << this->shift);
    reg_value |= value << this->shift;
    obj->_write_register(this->reg, reg_value);
}

_RegBits16::_RegBits16(int reg, int shift, int mask) {
    this->reg = reg;
    this->shift = shift;
    this->mask = mask;
}

int _RegBits16::__get__(OV5640* obj) {
    int reg_value = obj._read_register16(this->reg);
    return (reg_value >> this->shift) & this->mask;
}

void _RegBits16::__set__(OV5640* obj, int value) {
    if (value & ~this->mask) {
        Serial.printf("Value 0x%02x does not fit in mask 0x%02x\r\n", value, this->mask);
        return;
    }
    int reg_value = obj->_read_register16(this->reg);
    reg_value &= ~(this->mask << this->shift);
    reg_value |= value << this->shift;
    obj->_write_register16(this->reg, reg_value);
}


_SCCB16CameraBase::_SCCB16CameraBase(int i2c_address) {
    this->_address = i2c_address;
}

void _SCCB16CameraBase::_write_register(int reg, int value) {
    byte b[3];
    b[0] = reg >> 8;
    b[1] = reg & 0xFF;
    b[2] = value;

    Wire.begin(this->address);
    for (int i = 0; i < 3; i++) {
        Wire.write(b);
    }
    Wire.end();
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
    byte b[2]
    b[0] = reg >> 8;
    b[1] = reg & 0xFF;

    Wire.begin(this->address);

    Wire.write(b);
    while (Wire.available()) {
        byte[0] = Wire.read();
    }
    Wire.end();

    return b[0];
}

int _SCCB16CameraBase::_read_register16(int reg) {
    int high = this->_read_register(reg);
    int low = this->_reg_register(reg + 1);

    return (high << 8) | low;
}

void _SCCB16CameraBase::_write_list(int* reg_list, int reg_list_size) {
    for (int i = 0; i < reg_list_size, i = i + 2) {
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

OV5640::OV5640(int* data_pins, int clock, int vsync, int href, int shutdown = -1, int reset = -1, int mclk = -1, int mclk_frequency = 20000000, int i2c_address = 0x3C, int size = OV5640_SIZE_QQVGA) {
    this->data_pins = data_pins;
    this->clock = clock;
    this->vsync = vsync;
    this->href = href;
    this->shutdown = shutdown;
    this->reset = reset;
    this->mclk = mclk;
    this->mclk_frequency = mclk_frequency;
    this->i2c_address = i2c_address;
    this->size = size;

    this->chip_id = new _RegBits16(_CHIP_ID_HIGH, 0, 0xFFFF);

    // if (this->mclk != -1) {}

    this->_write_list(_sensor_default_regs, 272)
}

OV5640::~OV5640() {
    delete chip_id;
}

!!!!!!
// PARALLEL CAPTURE STUFF!!!!
void OV5640::capture() {
    return;
}
!!!!!!

int OV5640::capture_buffer_size(void) const {
    if (this->colorspace == OV5640_COLOR_JPEG) return this->width * this->height / this->quality;
    if (this->colorspace == OV5640_COLOR_GRAYSCALE) return this->width * this->height;
    return this->width * this->height * 2;
}

int OV5640::mclk_frequency(void) const {
    return 0;
}

int OV5640::width(void) const {
    return this->_w;
}

int OV5640::height(void) const {
    return this->_h
}

int OV5640::colorspace(void) const {
    return this->_colorspace;
}

void OV5640::colorspace(int colorspace) {
    this->_colorspace = colorspace;
    this->_set_size_and_colorspace();
}

void OV5640::_set_image_options(void) {
    int reg20(0), reg21(0), reg4514(0), reg4514_test(0);

    if (this->colorspace() == OV5640_COLOR_JPEG) reg21 |= 0x20;

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
    int* settings = _ov5640_color_settings[this->_colorspace];
    int size = _ov5640_color_settings_size[this->_colorspace];

    this->_write_list(settings, size);
}

int OV5640::size(void) const {
    return this->_size;
}

void OV5640::_set_size_and_colorspace(void) {
    int size = this->_size;
    int* _this_resolution_info = _resolution_info[size];
    int width = _this_resolution_info[0];
    int height = _this_resolution_info[1];
    int ratio = _this_resolution_info[2];

    this->_w = width;
    this->_h = height;

    
    int* _this_ratio_table = _ratio_table[ratio];

    int max_width = _this_ratio_table[0];
    int max_height = _this_ratio_table[1];
    int start_x = _this_ratio_table[2];
    int start_y = _this_ratio_table[3];
    int end_x = _this_ratio_table[4];
    int end_y = _this_ratio_table[5];
    int offset_x = _this_ratio_table[6];
    int offset_y = _this_ratio_table[7];
    int total_x = _this_ratio_table[8];
    int total_y = _this_ratio_table[9];

    this->_binning = (width <= max_width / 2) && (height <= max_height / 2);

    this->_scale = !((width == max_width && height == max_height) || (width == max_width / 2 && height == max_height / 2));

    this->_write_addr_reg(_X_ADDR_ST_H, start_x, start_y);
    this->_write_addr_reg(_X_ADDR_END_H, end_x, end, y);
    this->_write_addr_reg(_X_OUTPUT_SIZE_H, width, height);

    if (!(this->binning)) {
        this->_write_addr_reg(_X_TOTAL_SIZE_H, total_x, total_y);
        this->_write_addr_reg(_X_OFFSET_H, offset_x, offset_y);
    } else {
        if (width > 920) this->_write_addr_reg(_X_TOTAL_SIZE_H, total_x - 200, total_y / 2);
        else this->_write_addr_reg(_X_TOTAL_SIZE_H, 2060, total_y / 2);

        this->_write_addr_reg(_X_OFFSET_H, offset_x / 2, offset_y / 2);
    }

    this->_write_reg_bits(_ISP_CONTROL_01, 0x20, this->_scale);

    this->_set_image_options();

    if (this->colorspace() == OV5640_COLOR_JPEG) {
        int sys_mul = 200;
        if (size < OV5640_SIZE_QVGA) sys_mul = 160;
        if (size < OV5640_SIZE_XGA) sys_mul = 180;

        this->_set_pll(0, sys_mul, 4, 2, 0, 2, 1, 4);
    } else this->_set_pll(0, 32, 1, 1, 0, 1, 1, 4);

    this->_set_colorspace();

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

    this->_write_register(0x3039, bypass ? 0x80 : 0);
    this->_write_register(0x3034, 0x1A);
    this->_write_register(0x3035, 1 | ((sys_div & 0xF) << 4));
    this->_write_register(0x3036, multiplier & 0xFF);
    this->_write_register(0x3037, (pre_div & 0xF) | (root_2x ? 0x10 : 0));
    this->_write_register(0x3108, (pclk_root_div & 3) << 4 | 0x06);
    this->_write_register(0x3824, pclk_div & 0x1F);
    this->_write_register(0x460C, 0x22);
    this->_write_register(0x3103, 0x13);
}

void OV5640::size(int size) {
    this->_size = size;
    this->_set_size_and_colorspace();
}

bool OV5640::flip_x(void) const { return this->_flip_x; }
void OV5640::flip_x(bool value) {
    this->_flip_x = value;
    this->_set_image_options();
}

bool OV5640::flip_y(void) const { return this->_flip_y; }
void OV5640::flip_y(bool value) {
    this->_flip_y = value;
    this->_set_image_options();
}

bool OV5640::test_pattern() const { return this->_test_pattern; }
void OV5640::test_pattern(bool value) {
    this->_test_pattern = value;
    this->_write_register(_PRE_ISP_TEST_SETTING_1, value << 7);
}

int OV5640::saturation(void) const { return this->_saturation; }
void OV5640::saturation(int value) {
    if (!(value >= -4 && value <= 4)) {
        Serial.printf("Invalid saturation %d, use value from -4..4 inclusive\r\n", value);
        return;
    }
    int* _this_sensor_saturation_levels = _sensor_saturation_levels[value + 4];

    for (int offset = 0; offset < 11; offset++) {
        int reg_value = _this_sensor_saturation_levels[i];
        this->_write_register(0x5381 + offset, reg_value);
    }
    this->_saturation = value;
}

int OV5640::effect(void) const { return this->_effect; }
void OV5640::effect(int value) {
    int reg_addr[4] = { 0x5580, 0x5583, 0x5584, 0x5003 };
    int* _this_sensor_special_effects = _sensor_special_effects[value];
    for (int i = 0; i < 4; i++) {
        int reg_value = _this_sensor_special_effects[i];
        this->_write_register(reg_addr[i], reg_value);
    }
    this->_effect = value;
}

int OV5640::quality(void) { return this->_read_register(_COMPRESSION_CTRL07) & 0x3F; }
void OV5640::quality(int value) {
    if (!(value >= 2 && value <= 55)) {
        Serial.printf("Invalid quality value %d, use a value from 2..55 inclusive\r\n", value);
        return;
    }
    this->_write_register(_COMPRESSION_CTRL07, value & 0x3F);
}

void OV5640::_write_group_3_settings(int* settings) {
    this->_write_register(0x3212, 0x3);
    this->_write_list(settings, 4);
    this->_write_register(0x3212, 0x13);
    this->_write_register(0x3212, 0xA3);
}

int OV5640::brightness(void) {
    int brightness_abs = this->_read_register(0x5587) >> 4;
    int brightness_neg = this->_read_register(0x5588) & 8;

    if (brightness_neg) return -brightness_abs;
    return brightness_abs;
}
void OV5640::brightness(int value) {
    if (!(value >= -4 && value <= 4)) {
        Serial.printf("Invalid brightness value %d, use a value from -4..4 inclusive\r\n", value);
        return;
    }
    int temp[4] = { 0x5587, abs(value) << 4, 0x5588, value < 0 ? 0x9 : 0x1 };

    this->_write_group_3_settings(temp);
}

int OV5640::contrast(void) {
    int contrast_abs = this->_read_register(0x5587) >> 4;
    int contrast_neg = this->_read_register(0x5588) & 8;

    if (contrast_neg) return -contrast_abs;
    return contrast_abs;
}
void OV5640::contrast(int value) {
    if (!(value >= -3 && value <= 3)) {
        Serial.printf("Invalid contrast value %d, use a value from -3..3 inclusive\r\n", value);
        return;
    }
    int* setting = _contrast_settings[value + 3];
    int temp[4] = { 0x5586, setting[0], 0x5585, setting[1] };
    this->_write_group_3_settings(temp);
}

int OV5640::exposure_value(void) const { return this->_ev; }
void OV5640::exposure_value(int value) {
    if (!(value >= -3 && value <= 3)) {
        Serial.printf("Invalid exposure value %d, use a value from -3..3 inclusive\r\n", value);
        return;
    }
    int* _this_sensor_ev_values = _sensor_ev_levels[value + 3];

    for (int offset = 0; offset < 6; offset++) {
        int reg_value = _this_sensor_ev_values[i];
        this->_write_register(0x5381 + offset, reg_value);
    }
    this->_ev = value;
}

int OV5640::white_balance(void) const { return this->_white_balance; }
void OV5640::white_balance(int value) {
    if (!(value >= OV5640_WHITE_BALANCE_AUTO  && value <= OV5640_WHITE_BALANCE_INCANDESCENT)) {
        Serial.printf("Invalid white balance value %d, use one of the OV5640_WHITE_BALANCE_* constants", value);
        return;
    }
    this->_write_register(0x3212, 0x3);
    int* _this_light_mode = _light_modes[value];
    for (int i = 0; i < 7; i++) {
        int reg_addr = _light_registers[i];
        int reg_value = _this_light_mode[i];
        this->_write_register(reg_addr, reg_value);
    }
    this->_write_register(0x3212, 0x13);
    this->_write_register(0x3212, 0xA3);
}

bool OV5640::night_mode(void) { return bool(this->_read_register(0x3A00) & 0x04); }
void OV5640::night_mode(bool value) { this->_write_reg_bits(0x3A00, 0x04, value); }
