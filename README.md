# Nuertey-DHT11-Mbed

An ARM Mbed application that illustrates how a NUCLEO-F767ZI can be 
connected to a DHT11 Temperature and Humidity sensor and values 
output to an LCD 16x2 display, all mocked-up via a breadboard. This
allows us to periodically obtain temperature and humidity readings.

Furthermore, the application continuously blinks 3 10mm external 
LEDs connected to various I/O ports at different frequencies.

And lastly, the application employs PWM output pins on the 
NUCLEO-F767ZI to vary the intensity of 3 other 10mm LEDs according to 
an on-the-fly calculated sawtooth waveform pattern and, precomputed 
triangular and sinusoidal waveform function values. Essentially,  
the values of said waveforms are scaled and used as the duty cycle
in driving, dimming, and brightening the 10mm LEDs.

![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_dashboard_1.jpeg?raw=true)
![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_dashboard_2.jpeg?raw=true)
![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_dashboard_3.jpeg?raw=true)

![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_project_5.jpeg?raw=true)
![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_project_2.jpeg?raw=true)
![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_project_4.jpeg?raw=true)
![alt text](https://github.com/nuertey/RandomArtifacts/blob/master/climate_project_6.jpeg?raw=true)

## License
MIT License

Copyright (c) 2021 Nuertey Odzeyem

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
