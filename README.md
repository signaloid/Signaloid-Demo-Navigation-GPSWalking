[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-dark-v6.png#gh-dark-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Navigation-GPSWalking#gh-dark-mode-only)
[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-light-v6.png#gh-light-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Navigation-GPSWalking#gh-light-mode-only)

# MICRO Benchmark: `speed-calc-GPS`

Benchmark from Tsoutsouras et al. MICRO paper.[^0]

This is the "GPS Walking" example from the Uncertain\<T\> work [^1].

## Overview
The benchmark uses noisy GPS measurements to calculate the GPS receiver's speed.

## Arguments

```
speed-calc-GPS -f <file> -s <samples> -m <mode>
	-f <file>: input (`sensoringData_gps_clean_user1_walking_driving-uncertainT-64.csv` is the default)
	-s <samples>: number of samples per value (32 is the default)
	-m <mode>: 1 for explicit computation, 0 for implicit uncertainty tracking (0 is the default)
```


## Inputs

The inputs are part of a public dataset for activity tracking [^2] distributed under a CC-BY 4.0 LICENSE.

[^0]: Vasileios Tsoutsouras, Orestis Kaparounakis, Bilgesu Arif Bilgin, Chatura Samarakoon, James Timothy Meech, Jan Heck, Phillip Stanley-Marbell: The Laplace Microarchitecture for Tracking Data Uncertainty and Its Implementation in a RISC-V Processor. MICRO 2021: 1254-1269

[^1]: J. Bornholt, T. Mytkowicz, and K. S. McKinley, “Uncertain\<T\>: a first-order type for uncertain data,” in Proceedings of the 19th international conference on Architectural support for programming languages and operating systems - ASPLOS ’14, Salt Lake City, Utah, USA, 2014, pp. 51–66. doi: 10.1145/2541940.2541958.

[^2]: Garcia-Gonzalez, Daniel; Rivero, Daniel; Fernandez-Blanco, Enrique; R. Luaces, Miguel (2020), “A Public Domain Dataset for Real-Life Human Activity Recognition Using Smartphone Sensors”, Mendeley Data, V1, doi: 10.17632/3xm88g6m6d.1

