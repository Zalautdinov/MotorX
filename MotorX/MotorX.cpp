#include <MotorX.h>

#if defined(ESP8266) || defined(ARDUINO_ESP8266_NODEMCU) || defined(__AVR_ATmega328P__)

/**
 * @brief Создаем новый мотор 
 * 
 * @param chip тип микросхемы драйвера коллекторного мотора ( L9110=0, TA6586=0, VHN2SP30=1 )
 * @param In1 1 порт входного сигнала для управления скоростью ( PWM )
 * @param In2 2 порт входного сигнала для управления скоростью ( PWM )
 * @param InPwm порт шим сигнала только для микросхемы VHN2SP30
 */
void MotorX::begin(byte chip, byte In1, byte In2, byte InPwm)
{
    dr_chip = chip;
    port_in1 = In1;
    port_in2 = In2;
    port_pwm = InPwm;
}
#endif

#if defined(ESP32)
/**
 * @brief Создаем новый мотор 
 * 
 * @param chip тип микросхемы драйвера коллекторного мотора ( L9110=0, TA6586=0, VHN2SP30=1 )
 * @param In1 1 Первый порт входного сигнала для управления скоростью ( PWM )
 * @param canal_in1 Номер первого шим канала только для ESP32
 * @param In2 2 Второй порт входного сигнала для управления скоростью ( PWM )
 * @param canal_in2 Номер второго шим канала только для ESP32
 * @param InPwm порт шим сигнала только для микросхемы VHN2SP30
 * @param canal_InPwm Номер шим канала только для микросхемы VHN2SP30
 */
void MotorX::begin(byte chip, byte In1, byte canal_in1, byte In2, byte canal_in2, byte InPwm, byte canal_InPwm)
{
    dr_chip = chip;
    port_in1 = In1;
    port_in2 = In2;
    port_pwm = InPwm;

    if (dr_chip == L9110 || dr_chip == TA6586)
    {
        ledcAttachPin(In1, canal_in1);
        ledcAttachPin(In2, canal_in2);
        ledcSetup(canal_in1, 5000, 8);
        ledcSetup(canal_in2, 5000, 8);
        canal1 = canal_in1;
        canal2 = canal_in2;
    }

    if (dr_chip == VHN2SP30)
    {
        ledcAttachPin(InPwm, canal_InPwm);
        ledcSetup(canal_InPwm, 5000, 8);
        canal1 = canal_InPwm;
    }
}

#endif

/**
 * @brief Запуск вращения двигателя с плавным ускорением или на прямую от pwm
 * 
 * @param dir_in Направление вращения мотора
 * @tparam dir=2 холостой ход
 * @tparam dir=0 направление вперед
 * @tparam dir=1 направление назад
 * @tparam dir=3 тормоз двигателя если потдерживает драйвер
 * 
 * @param pwm Шим сионал - управление скоростью вращения 
 * @param inc ускорение от 0 до pwm (если inc = pwm скорость равна pwm)
 */
void MotorX::On(byte dir_in, byte pwm, byte inc)
{

    if (speed == 0 && dir_in < 2)
        dir = dir_in;

    if (dir_in != dir)
    {
        pwm = constrain(speed - 1, 0, 254);
    }
    if (inc == pwm)
        speed = pwm;
    else
    {
        speed = speed + (inc * (speed < pwm)) - (inc * (speed > pwm));
    }
    speed = constrain(speed, 0, 254);

    if (dr_chip == L9110 || dr_chip == TA6586)
    {
        if (dir_in == 2)
            WriteMotor(0, 0);
        if (dir_in == 3)
            WriteMotor(254, 254);
        else
            WriteMotor(pwm * dir, pwm * !dir);
    }
    if (dr_chip == VHN2SP30)
    {
        if (dir_in == 2)
        {
            digitalWrite(port_in1, 0);
            digitalWrite(port_in2, 0);
        }
        if (dir_in == 3)
        {
            digitalWrite(port_in1, 1);
            digitalWrite(port_in2, 1);
        }
        else
        {

            Serial.println(String(dir) + "  " + String(dir_in) + "  " + String(speed) + "  " + String(pwm));
            digitalWrite(port_in1, dir);
            digitalWrite(port_in2, !dir);
#if defined(ESP8266) || defined(ARDUINO_ESP8266_NODEMCU) || defined(__AVR_ATmega328P__)
            analogWrite(port_pwm, speed);
#endif
#if defined(ESP32)
            ledcWrite(canal1, speed);
#endif
        }
    }
}
void MotorX::WriteMotor(byte pwm1, byte pwm2)
{
#if defined(ESP8266) || defined(ARDUINO_ESP8266_NODEMCU) || defined(__AVR_ATmega328P__)
    analogWrite(port_in1, pwm1);
    analogWrite(port_in2, pwm2);
#endif
#if defined(ESP32)
    ledcWrite(canal1, pwm1);
    ledcWrite(canal2, pwm2);
#endif
}

/**
 * @brief Поворот сервопривода на указанный угол
 * 
 * @param t Угол поворота в градусах от 0 до 180
 * @param inc Величина-скорость наращивания угла до указано в параметре "t"
 * @tparam inc Если равен нулю, то угол устанавливается мгновенно до  указанного
 * @tparam  Вызов функции без параметров установит последний установленный угол поворота
 * @tparam Который можно получить командой Servo.Read() 
 */
void ServoX::Write(byte t, byte inc)
{
    if (mode)
    {
        inc = constrain(inc, 0, 180);
        if (t != 254)
            ugol = t;
        if (inc > 0)
        {
            c_ugol = c_ugol + (inc * (c_ugol < ugol)) - (inc * (c_ugol > ugol));
            c_ugol = constrain(c_ugol, 0, ugol);
        }
        else
        {
            c_ugol = ugol;
        }

        t = map(constrain(c_ugol, 0, 180), 0, 180, 540, 2400);
        digitalWrite(port, 1);
        delayMicroseconds(t);
        digitalWrite(port, 0);
        delayMicroseconds(20000 - t);
    }
}
/**
 * @brief Установка параметров сервопривода
 * 
 * @param p Цифровой порт подключения сервопривода
 */
void ServoX::Attach(byte p)
{
    port = p;
    mode = true;
    pinMode(port, OUTPUT);
}
/**
 * @brief Разрешает работу серво привода
 * 
 */
void ServoX::On()
{
    mode = true;
}
/**
 * @brief Отключает работу серво привода 
 * 
 */
void ServoX::Off()
{
    mode = false;
}
/**
 * @brief Возвращает установленный угол на серво приводе 
 * 
 * @return byte Угол в градусах
 */
byte ServoX::Read()
{
    return ugol;
}
/**
 * @brief Возвращает текущий режим работы серво привода
 * 
 * @retval True Серво привод включен
 * @retval False Серво привод отключен
 */
bool ServoX::ReadMode()
{
    return mode;
}
