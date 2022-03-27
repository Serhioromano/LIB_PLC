#include <stdint.h>
#include <stdbool.h>
#include <iso646.h>
#include "FbPIDcontrol.h"

#define ProcessVariable p->ProcessVariable
#define Setpoint        p->Setpoint
#define Kp              p->Kp
#define Ki              p->Ki
#define Kd              p->Kd
#define Kdf             p->Kdf
#define DBmax           p->DBmax
#define DBmin           p->DBmin
#define OutMax          p->OutMax
#define OutMin          p->OutMin
#define Ts              p->Ts
#define Manual          p->Manual
#define ManOn           p->ManOn
#define Out             p->Out
#define Er              p->Er
#define Ppart           p->Ppart
#define Ipart           p->Ipart
#define Dpart           p->Dpart
#define Dintegral       p->Dintegral

void FbPIDcontrol(struct DbPIDcontrol *p)
{

  //Временные переменные, не сохраняемые.
  float Auto; //Выход ПИД- алгоритма и вход переключателя режимов работы.
  float Sw  ; //Выход с переключателя режимов работы.

  //Ошибка регулирования.
  Er = Setpoint - ProcessVariable;

  //Зона нечувствительности к ошибке.
  if ((DBmin < Er) and (Er < DBmax))
  {
    Er = 0.0;
  }

  //Пропорциональная составляющая.
  Ppart = Kp * Er;

  //Интегральная составляющая.
  if (0.0 == Ki)
  {
    Ipart = 0.0;
  }
  else
  {
    Ipart = Ipart + Ki * Er * Ts;
  }

  //Дифференциальная составляющая.
  if (0.0 == Kd)
  {
    Dpart = 0.0;
    Dintegral = 0.0;
  }
  else
  {
    Dpart = (Er * Kd - Dintegral) * Kdf;
    Dintegral = Dintegral + Dpart * Ts;
  }

  //Сигнал управления.
  Auto = Ppart + Ipart + Dpart;

  //Переключение режима работы "Ручной / Автоматический".
  if (ManOn)
  {
    Sw = Manual;
  }
  else
  {
    Sw = Auto;
  }

  //Амплитудный ограничитель- максимум.
  if (Sw >= OutMax)
  {
    Out = OutMax;
    //Ограничение интегральной составляющей методом обратного вычисления.
    Ipart = Out - (Ppart + Dpart);
  }

  //Амплитудный ограничитель- минимум.
  if (Sw <= OutMin)
  {
    Out = OutMin;
    //Ограничение интегральной составляющей методом обратного вычисления.
    Ipart = Out - (Ppart + Dpart);
  }

  //Амплитудный ограничитель- без ограничений.
  if ((OutMin < Sw) and (Sw < OutMax))
  {
    Out = Sw;
    if (ManOn)
    {
      //Ограничение интегральной составляющей методом обратного вычисления.
      Ipart = Out - (Ppart + Dpart);
    }
  }

  return;
}

// Передаточная функция регулятора (при Ts -> 0):
//
//         Out(s)               1              s
// W(s) = -------- = Kp + Ki * --- + Kd * -------------
//          Er(s)               s          Tdf * s + 1
//
// Er(s) = Setpoint(s) - ProcessVariable(s).
// Tdf = 1 / Kdf

// Передаточная функция интегратора:
//
//         Ts * z
// W(z) = --------
//         z - 1
//
// Численное интегрирование методом прямоугольников.
// y = y + x * Ts;

// Перечень сокращений.
//
// Fb          - Function block       - Функциональный блок.
// Db          - Data block           - Блок данных.
// PIDcontrol  - PID Controller       - ПИД- регулятор (пропорционально интегрально дифференциальный регулятор).
// SP          - Setpoint             - Заданное значение.
// PV          - Process Variable     - Измеренное значение.
// OP          - Output PIDcontroller - Сигнал управления.
// Er          - Error                - Ошибка регулирования, рассогласование.
// DBmax       - Dead band maximum    - Максимум зоны нечувствительности.
// DBmin       - Dead band minimum    - Минимум зоны нечувствительности.
// OPmax       - Output power maximum - Максимум сигнала управления.
// OPmin       - Output power minimum - Минимум сигнала управления.
// Ts          - Sample Time          - Шаг дискретизации по времени.
// ManOn       - Manual on            - Включить ручной режим управления.
// Sw          - Switch               - Переключатель.

// Характеристики ПИД- регулятора.
//
// Ограничение интегральной составляющей методом обратного вычисления.
// Зона нечувствительности к ошибке регулирования для устранения небольших колебаний регулятора в установившемся режиме работы.
// Амплитудное ограничение выходного сигнала для предотвращения подачи сигнала управления, который превышает физические возможности исполнительного механизма.
// Встроенный в дифференциальную составляющую фильтр низких частот, для устранения дифференцирования шумов измерения.
// Фильтр шума- дискретное апериодическое звено первого порядка.
// Переключение режимов управления РУЧНОЙ / АВТОМАТ.
// Безударный переход из ручного режима в автоматический.
// Безударный переход из автоматического режима в ручной.
// Принудительное обнуление интегратора интегральной составляющей при Ki=0.
// Принудительное обнуление интегратора дифференциальной составляющей при Kd=0.
// Численное интегрирование методом прямоугольников.
// Нет ни одной операции деления.
// Протестировано на PLC SIEMENS SIMATIC S7-300 в блоке OB35 (Ts=0,1c).

// Известные недостатки ПИД- регулятора.
//
// При переходе из автоматического режима в ручной безударный переход отсутствует, если не подвязать через внешний тег переменную выхода OP к входу MANUAL.
// Безударный переход работает только если Ki не равен нолю.
// Необходимо следить, чтобы верхняя граница зоны нечувствительности была больше или равна нижней.
// Необходимо следить, чтобы верхняя граница амплитудного ограничителя была больше нижней.
// Необходимо следить, чтобы коэффициент Kdf был больше ноля.
// Необходимо помнить, что коэффициент Kdf обратно пропорционален постоянной времени фильтра.
// Прямоугольный интегратор имеет меньшую точность, чем трапецеидальный.
// Арифметика с плавающей точкой работает медленнее целочисленной.
// Точность вычислений не контролируется.
// Необходимо следить при наладке, чтобы не было слишком малых измеренных значений и интегральной составляющей при большом выходном сигнале.
// Иначе интегрирование может остановиться и за того, что мы пытаемся прибавить очень маленькое число к очень большому, а точность типа данных не позволяет это сделать.
// ПИ- закон регулирования с апериодическим объектом управления по определе-нию не может безошибочно «отрабатывать» линейно нарастающий сигнал задания.
// Сигналы SP и PV нужно ограничивать вне регулятора,
// диаппазоны ограничения для SP и PV должны быть одинаковыми,
// если этого не сделать то будут проблемы в автоматическом режиме работы при активной работе амплитудного ограничителя,
// например SP=90% PV=110% ожидаем Out=0%, но при изменении PV возрастает Out хотя должно быть Out=0%.

//Структура регулятора:
//
// Ошибка регулирования.
//                   +---+
//                   |   |
//        Setpoint->-|+  |
//                   |   |->-Er
// ProcessVariable->-|-  |
//                   |   |
//                   +---+
//
// Зона нечувствительности к ошибке.
//          DBmax
//      +-----------+
//      |           |
//      |       /   |
//      |      /    |
//      |      |    |
// Er->-|   +--+    |->-Er
//      |   |       |
//      |  /        |
//      | /         |
//      |           |
//      +-----------+
//          DBmin
//
// Пропорциональная составляющая.
//      +---+
//      |   |
// Er->-|   |
//      | * |->-Ppart
// Kp->-|   |
//      |   |
//      +---+
//
// Интегральная составляющая.
//      +---+   +----------+
//      |   |   |          |
// Er->-|   |   |  Z * Ts  |
//      | * |->-| -------- |->-Ipart
// Kp->-|   |   |  Z - 1   |
//      |   |   |          |
//      +---+   +----------+
//
// Дифференциальная составляющая.
//                        +---+
//                        |   |
//      +---+       Kdf->-|   |
//      |   |     +---+   |   |
// Er->-|   |     |   |   | * |----------------+-->-Dpart
//      | * |--->-|+  |   |   |                |
// Kd->-|   |     |   |->-|   |                |
//      |   | +->-|-  |   |   | +----------+   |
//      +---+ |   |   |   +---+ |          |   |
//            |   +---+         |  Z * Ts  |   |
//            +-----------------| -------- |-<-+
//                              |  Z - 1   |
//                              |          |
//                              +----------+
//
// Сигнал управления.
//         +---+
//         |   |
// Ppart->-|+  |
//         |   |
// Ipart->-|+  |->-Auto
//         |   |
// Dpart->-|+  |
//         |   |
//         +---+
//
// Переключение режима работы "Ручной / Автоматический".
//          +-----+
//          |     |
// Auto--->-|-+   |
//          |  \  |
//          |   +-|->-Sw
//          |     |
// Manual->-|-+   |
//          |     |
//          +-----+
//             |
// ManOn-->----+
//
// Амплитудный ограничитель.
//         OutMax
//      +-----------+
//      |           |
//      |       +-- |
//      |      /    |
// Sw->-|     /     |->-Out
//      |    /      |
//      | --+       |
//      |           |
//      +-----------+
//         OutMin


// Реакция разомкнутого контура регулятора на единичное спупечатое воздействие по каналу управления.
//
// ^ Er
// |
// 1************ Er(t) = 1
// |
// 0---1---2---3---> t[s]
//
// ^ Ppart
// |
// |************ Ppart(t) = Kp * Er
// |
// |
// |
// 0---1---2---3---> t[s]
//
// ^ Ipart
// |
// 3           * Ipart(t) = Ki * t
// |         *
// 2       *
// |     *
// 1   *
// | *
// 0---1---2---3---> t[s]
//
// ^ Dpart
// |
// 1*            Dpart(t) = Kd * (0 - exp(-(t/Tf)))
// | *
// |  *
// |    *
// |      *
// 0---------***---> t[s]
//
// ^ Out
// |
// |                               *
// |                             *
// |                           *
// |                         *
// |                       *
// |                     *
// |*                  *
// | *               *
// |  *            *
// |    *        *
// |      *    *
// |        **
// |
// |
// 0------------------------------------> t[s]

// Интерфейс пользователя SCADA, HMI.
//
// Шрифт: Arial 12pt
// Цвет фона: R212 G208 B200
// Цвет тени: R169 G169 B169
// Цвет "Заданное значение"  : R0 G0 B255
// Цвет "Измеренное значение": R0 G176 B80
// Цвет "Сигнал управления"  : R255 G0 B0
//
// +------------------------------------------------------+---+
// | 1PIRCA1 ПИД Регулятор давления воды.                 | X |
// +------------------------------------------------------+---+
// | +-------+ +--------------------------------------------+ |
// | | 10.00 | | Заданное значение           0...10 [Bar] B | |
// | +-------+ +--------------------------------------------+ |
// | +-------+ +--------------------------------------------+ |
// | | 10.01 | | Измеренное значение         0...10 [Bar] G | |
// | +-------+ +--------------------------------------------+ |
// | +-------+ +--------------------------------------------+ |
// | |  50.0 | | Сигнал управления           0...100 [Hz] R | |
// | +-------+ +--------------------------------------------+ |
// | +---------+ +--------+ +------+ +--------+ +-----------+ |
// | | АВТОМАТ | | РУЧНОЙ | | СТОП | | ТРЕНДЫ | | НАСТРОЙКИ | |
// | +---------+ +--------+ +------+ +--------+ +-----------+ |
// | Код ошибки: 0 (OK).                                      |
// +----------------------------------------------------------+
//
// +-------------------------------------------+---+
// | 1PIRCA1 Тренды.                           | X |
// +-------------------------------------------+---+
// | Заданное значение   [Bar] B                   |
// | Измеренное значение [Bar] G                   |
// | Сигнал управления   [Hz ] R                   |
// |   ^                                           |
// |   |                                           |
// |100+                                           |
// |   |                                           |
// |   |            **********************         |
// |   |           *                               |
// |   |          *                                |
// | 50+    +----*------------------------         |
// |   |    |   *  +++++++++++++++++++++++         |
// |   |    |  *  +                                |
// |   |    | *  +                                 |
// |   |    |*  +                                  |
// |   +----+----+----+----+----+----+----+-> t[s] |
// |   0   10   20   30   40   50   60   70        |
// +-----------------------------------------------+
//
// +-------------------------------------------+---+
// | 1PIRCA1 Настройки ПИД регулятора.         | X |
// +-------------------------------------------+---+
// | +-------+ +---------------------------------+ |
// | | 0.001 | | Kp                              | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | | 0.001 | | Ki                              | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | |   0.0 | | Kd                              | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | | 1.000 | | Kdf                             | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | | 0.001 | | DBmax                           | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | |-0.001 | | DBmin                           | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | | 100.0 | | OutMax                          | |
// | +-------+ +---------------------------------+ |
// | +-------+ +---------------------------------+ |
// | |   0.0 | | OutMin                          | |
// | +-------+ +---------------------------------+ |
// +-----------------------------------------------+

//  +---------+
//  | GNU GPL |
//  +---------+
//  |
//  |
//  .= .-_-. =.
// ((_/)o o(\_))
//  `-'(. .)`-'
//  |/| \_/ |\
//  ( |     | )
//  /"\_____/"\
//  \__)   (__/
