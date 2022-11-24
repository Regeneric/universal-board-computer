package main

import (
	"machine"
	"time"
)

var pulseDistance float32 = 0.00006823
var injectionValue float32 = 0.0025

// float
var avgSpeedDivider, traveledDistance, sailingDistance float32 = 0.0, 0.0, 0.0
var sumInv, accTime float32 = 0.0, 0.0

var injectorPulseTime float32 = 0.0
var instantFuelConsumption, averageFuelConsumption float32 = 0.0, 0.0
var usedFuel, fuelLeft, fuelSumInv float32 = 0.0, 0.0, 0.0
var divideFuelFactor, savedFuel float32 = 0.0, 0.0

var injectorOpenTime, ccMin float32 = 0.0, 100.0

// uint8
var saveCounter, btnCnt, calibrationFlag uint8 = 60, 24, 0
var pulseOverflows, mode, accBuffer uint8 = 0, 3, 0

var speed, avgSpeedCount int = 0, 0

// int
var counter, distPulseCount int = 4, 0

// var injTimeHigh, injTimeLow, rangeDistance int = 0, 0, 0
var rangeDistance int = 0

// Speed functions
var avgSpeed = func() {
	sumInv += 1.0 / float32(speed)
	avgSpeedCount = int(avgSpeedDivider) / int(sumInv)
}
var currentSpeed = func() { speed = int(pulseDistance) * int(distPulseCount) * 3600 }

// Fuel functions
var fuelConsumption = func() {
	iotv := (injectorOpenTime * injectionValue) * 21600 // 21600 because 3600 (seconds in hour) * 6 (no. of injectors)
	inv := (injectorOpenTime * injectionValue) * 6

	injectorOpenTime = (float32(injectorPulseTime) / 1000) // Converting to seconds
	if speed > 5 {
		instantFuelConsumption = (100 * iotv) / float32(speed)

		// Harmonic mean
		if instantFuelConsumption > 0 && instantFuelConsumption < 100 {
			fuelSumInv += 1.0 / instantFuelConsumption
			averageFuelConsumption = avgSpeedDivider / fuelSumInv
		}
	} else {
		instantFuelConsumption = iotv
	}

	usedFuel += inv
	fuelLeft -= inv
}

var modeCounter int = 0

func main() {
	navBtn := machine.GP4
	navBtn.Configure(machine.PinConfig{Mode: machine.PinInputPullup})

	vssSignal := machine.GP2
	vssSignal.Configure(machine.PinConfig{Mode: machine.PinInputPullup})

	injSignal := machine.GP3
	injSignal.Configure(machine.PinConfig{Mode: machine.PinInputPullup})

	// Nav button interrupt
	navBtn.SetInterrupt(machine.PinFalling,
		func(p machine.Pin) {
			if p.Get() == false {
				modeCounter++
			}
			if modeCounter > 2 {
				modeCounter = 0
			}
		},
	)

	// Speed signal interrupt
	vssSignal.SetInterrupt(machine.PinFalling|machine.PinRising,
		func(p machine.Pin) {
			println("[DEBUG] VSS Interrupt")
			println("[DEBUG] Calibration flag:", calibrationFlag)
			distPulseCount++

			if calibrationFlag > 0 && distPulseCount == 65535 {
				pulseOverflows++
			}

			traveledDistance += pulseDistance
			if instantFuelConsumption <= 0 {
				sailingDistance += pulseDistance
			}

			println("[DEBUG] Traveled distance:", traveledDistance)
		},
	)

	// Injector signal interrupt
	injSignal.SetInterrupt(machine.PinFalling,
		func(p machine.Pin) {
			println("[DEBUG] INJ Interrupt")
			injTimeLow := time.Now()

			if p.Get() == false {
				injTimeLow = time.Now()
			} else {
				injTimeHigh := time.Now()
				injectorPulseTime = injectorPulseTime + ((float32(injTimeHigh.Nanosecond()) - float32(injTimeLow.Nanosecond())) / 1000000)
				p.Set(true)

				println("[DEBUG] Injector pulse time:", injectorPulseTime)
			}
		},
	)

	for {
		counter--
		if counter <= 0 {
			currentSpeed()
			fuelConsumption()

			if speed > 5 {
				avgSpeedDivider++
				avgSpeed()
			}

			injectorPulseTime = 0.0
			counter = 4
		}

		time.Sleep(time.Millisecond * 250)

		switch modeCounter {
		case 0:
			println("[DEBUG] Used fuel:", usedFuel)
			println("[DEBUG] Sailing distance:", sailingDistance)
			println("[DEBUG] Fuel left:", fuelLeft)
		case 1:
			println("[DEBUG] Current speed:", speed)
		case 2:
			println("[DEBUG] Instant consumption", instantFuelConsumption)
		}
	}
}
