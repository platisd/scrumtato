# Scrumtato
Scrumtato is an ATtiny85-based gadget to keep overly passionate developers from excessively talking during daily stand-up meetings. The name stems from the combination of [SCRUM](https://en.wikipedia.org/wiki/Scrum_(software_development)) and [Hot Potato](https://en.wikipedia.org/wiki/Hot_potato_(game)).

## About Scrumtato
During our daily stand-up meetings (SCRUM), as we are a team full of talented and passionate developers, we just love to talk too much. This stretches our meeting and keeps us from getting the most out of our working hours. To solve this problem, here comes **Scrumtato: A gadget to make daily stand-ups agile again!**

![scrumtatos](https://platis.solutions/blog/wp-content/uploads/2017/06/scrumtato_featured.jpg)

## How does it work
Operating the Scrumtato is simple!
1. Press the button when you are ready to talk. You will hear 2 beeps.
2. Start talking. You have to concisely explain what you did since the last meeting and what you are planning to do until the next.
3. If you finish before the time runs out (in which case the Scrumtato will beep and vibrate), press the button again. You will hear 3 beeps.

### More details
The user begins interacting with the Scrumtato by pressing the on-board button and starts talking about all the extraordinary things they got into since the last daily stand-up. This initiates a countdown timer that lasts as long as each team member is supposed to speak. To indicate that half of the remaining time has just been depleted, Scrumtato will beep once. For example, if the total available period for someone to speak is 1 minute, the user will hear beeps on approximately the 30th, 45th, 52nd, 56th, 58th and 59th seconds. As soon as the time is up, a characteristic beep sequence is played and Scrumato vibrates.

In other words, Scrumtato can be considered as a stress ball that... actually stresses you out! If the team member finishes talking in a timely manner, then they merely need to press the button again and a sound sequence that indicates the successful end of a turn is played.

From a technical perspective, since the users should not have to change the battery often, the microcontroller will go to sleep as much as possible. That is between "games" and between beeps or vibrations during the "games". In the former case, the microcontroller is woken up by the button press and in the latter by a watchdog timer.

## Other uses
Scrumtato was initially conceived as a toy for kids (or grown ups, no judging) and particularly, as a way to play Hot Potato. This use case is supported separately from the main one, however their integration into a common program should not be complicated as they share logic and states.

Specifically, the watchdog timer is triggered faster and the buzzer beeps when 5% of the remaining time is depleted, therefore more frequently compared to the stand-up scenario. Additionally, the on-board analog accelerometer is utilized to determine if a player does not pass the ball fast enough. In that case, a faster countdown is initiated and the ball must be passed in a couple of seconds otherwise it "explodes" in the hands of the one that was delaying the game. This increases the suspense and the pace of the game.

## Software
The use cases supported by Scrumtato can be found in the [firmware](https://github.com/platisd/scrumtato/tree/master/firmware) directory.

* [Standup Potato](https://github.com/platisd/scrumtato/tree/master/firmware/StandupPotato): The way to make daily stand-ups agile again
* [Hot Potato](https://github.com/platisd/scrumtato/tree/master/firmware/HotPotato): A modern and suspenseful take on the classic Hot Potato game

## Hardware
From a physical perspective, Scrumtato is comprised of a PCB and a 3D-printed case to protect the electronics as well as make passing around easier and more fun.

### PCB
The printed circuit board was designed with [Eagle CAD](https://www.autodesk.com/products/eagle/overview) and the latest `rev. 1` boards were fabricated by [Seeed Fusion](https://www.seeedstudio.com/fusion_pcb.html) and their [2 layer DRU](http://www.seeedstudio.com/document/rar/Seeed_Gerber_Generater_2-layer.zip) to produce the Gerber files. Using the above service you can preview your board, as well as any changes you made, before ordering it and have it nicely packaged and delivered to your address in just a week.

### 3D printed cases
Depending on the how you use Scrumtato, different case should be utilized. When used as a stand-up meeting gadget, it is *probably* not going to be thrown around. Therefore a case from common PLA plastic will work just fine. However, if kids come into the equation a flexible case is strongly suggested, since the gadget will be mishandled and dropped on the floor. After all, this is part of the fun. The flexible version was printed with a [ZYYX+ printer](http://www.zyyx3dprinter.com/), using their proFLEX filament. The PLA version was printed with a [Micro M3D printer](https://printm3d.com/themicro/). The models were sliced with Simplify3D. If you need more information on the specific Simplify3D profiles used, tips on how to print (e.g. when to use support etc) or the source of the models, please contact me.

* [Scrumtato PLA version](https://github.com/platisd/scrumtato/tree/master/physibles/pla) - To be used during the daily stand-up meetings.
* [Scrumtato Flex version](https://github.com/platisd/scrumtato/tree/master/physibles/flex) - To be used as a toy for kids.

### Components
* **[Scrumtato PCB](hardware/)** - You can also [order it from OSH Park](https://oshpark.com/shared_projects/GQ4w7Qkz) or [Seeed Studio](https://www.seeedstudio.com/Scrumtato-Make-daily-stand-ups-agile-again-g-1012504)
* **Four M3x10 flat head screws** - To mount the board. Longer ones (until M3x40) will also work.
* **Two M3x20 flat head screws** - To keep the upper and bottom part of the case together. Longer ones (until M3x40) will also work. **Only for the toy/flexible version!**
* **CR2032 battery** - To power everything up
* **CR2032 battery holder** - To keep the battery in place. Get the through-hole version.
* **ATtiny85-20PU** - If you only want to use it during stand-up meetings, then an **ATtiny25-20PU** will suffice.
* **DIP-8 socket** - Optional, but will probably make your life easier if you need to remove the microcontroller.
* **GY-61 accelerometer module** - Based on the ADXL335 chip. Try to get one that has its pins unsoldered.
* **5-pin angled male header** - Optional, unless you use an accelerometer which needs to be placed vertically relative to the board.
* **5-pin female header** - Optional, unless you use an accelerometer and need to remove it.
* **9mm Buzzer** - They are not the most common but take up less space. I might consider replacing them in the future with something easier to source.
* **22Ω 1206 resistor** - Used for the buzzer. Other low valued resistors will probably work as well.
* **SMD vibrating motor** - To make Scrumtato vibrate. The board is modeled after Sanyo NRS-2574 but all other models that fit the dimensions will work.
* **4-pin tactile switch** - To initiate (or finish occasionally) the game. Get the through-hole version.
* **2-pin angled switch** - As a reset button. Through-hole component.
* **LED 5mm** - Optional and rather unnecessary when using the flexible version since it is not visible.
* **220Ω 1206 resistor** - Optional unless the LED is used.
* **BC547 NPN transistor** - To control the vibration motor.
* **1KΩ 1206 resistor** - To drive the transistor.
* **Two 10KΩ 1206 resistors** - Used as pull ups. One for the microcontroller's reset pin and the other for the tactile switch.

## Articles
[Scrumtato: Make daily stand-ups agile again](https://platis.solutions/blog/2017/06/12/scrumtato-make-daily-stand-ups-agile-again/)
