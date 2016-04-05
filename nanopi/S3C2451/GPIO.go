/*
	Author: hiram

*/

package S3C2451

import (
	"fmt"
)

	const BaseGPIO = 0x56000000
	
	const (PA, PB, PC, PD, PE, PF, PG = 0, 1, 2, 3, 4, 5, 6)
	const (Fun_0, Fun_1, Fun_2, Fun_3 = 0, 1, 2, 3)
	const (INPUT, OUTPUT = 0, 1)
	const (PULLDOWN, PULLUP, PULLDEFAULT = 0, 1, 2)
	const (LOW, HIGH = 0, 1)
	
	type PORT struct {
		Port	uint8
		hMem	[]uint8

		GPIOCON			*uint32
		GPIODAT			*uint32
		GPIODEFAULT0		*uint32
		GPIODEFAULT1		*uint32
	}

func CreatePort(PORTx uint8) (*PORT, bool) {
	var Result bool = false

	if (PORTx > PG) { return nil, Result }
	
	port := &PORT{Port: PORTx}
	BaseADDR := BaseGPIO + (0x10 * uint32(PORTx))
	fmt.Println("BaseADDR ")
	fmt.Println(BaseADDR)
	port.hMem, Result = IS3C2451().GetMMap(BaseADDR)
	fmt.Println("port.hMem ")
	//fmt.Println(port.hMem)
	if !Result { return nil, Result }

	Reg := (0x10 * uint32(PORTx))//(BaseADDR & 0x0000000F)
	fmt.Println("Reg ")
	fmt.Println(Reg)
	port.GPIOCON, Result = 	IS3C2451().Register(port.hMem, Reg + 0x0000)

	port.GPIODAT, _ = 	IS3C2451().Register(port.hMem, Reg + 0x0004)

	port.GPIODEFAULT0, _ = 	IS3C2451().Register(port.hMem, Reg + 0x0008)
	port.GPIODEFAULT1, _ =	IS3C2451().Register(port.hMem, Reg + 0x000C)
	
	return port, Result
}

func FreePort(port *PORT) {
	if (port.hMem != nil) { 
		IS3C2451().FreeMMap(port.hMem)
		fmt.Println("Free")
	}
}

func (this *PORT) SetData(Data uint32) {
	*this.GPIODAT = Data
}

func (this *PORT) GetData() (uint32) {
	fmt.Println("GetData ")
	return *this.GPIODAT
}

func (this *PORT) SetCon(Con uint32) {
	*this.GPIOCON = Con
	fmt.Println("Set Con ")
}

func (this *PORT) GetCon() (uint32){
	fmt.Println("GetCon ")
	fmt.Println(*this.GPIOCON)
	return *this.GPIOCON 
}

func (this *PORT) SetDefault0(default0 uint32) {
	*this.GPIODEFAULT0 = default0
}

func (this *PORT) GetDefault0() uint32 {
	return *this.GPIODEFAULT0
}

func (this *PORT) SetDefault1(default1 uint32) {
	*this.GPIODEFAULT1 = default1
}

func (this *PORT) GetDefault1() uint32 {
	return *this.GPIODEFAULT1
}



/******************************************************************************/
	type GPIO struct {
		Port	*PORT
		Pin		uint8
		Bit		uint32
	}
	
func CreateGPIO(PORTx uint8, PINx uint8) (*GPIO, bool) {
	var Result bool = false
	
	if PINx > 32 { return nil, Result }
	
	gpio := &GPIO{}
	gpio.Port, Result = CreatePort(PORTx)
	if !Result { return nil, Result }
	fmt.Println("gpio.port.GPIOCON ")
	fmt.Println(gpio.Port.GPIOCON)
	fmt.Println("gpio.port.GPIODAT ")
	fmt.Println(gpio.Port.GPIODAT)
	gpio.Pin = PINx
	gpio.Bit = (0x1 << PINx)

	return gpio, Result
}

func FreeGPIO(gpio *GPIO) {
	FreePort(gpio.Port)
}


func (this *GPIO)GPIOFSetDir(IO uint8){
	conData := this.Port.GetCon()
	//fmt.Println("conData1 ")//216
	//fmt.Println(conData)//216
	switch (IO) {
		case INPUT: conData = conData & (^(uint32)(3 << (this.Pin* 2)))
		case OUTPUT:
 			conData = conData | (uint32)(1 << (this.Pin* 2 ))
			conData = conData & (^(uint32(1<<(this.Pin*2 +1))))
	}
	//fmt.Println("conData2 ")//216
	//fmt.Println(conData)//216
	*this.Port.GPIOCON = conData 
}

func (this *GPIO)GPIOFSetData(Data uint8){
	switch (Data) {
		case HIGH: *this.Port.GPIODAT |= this.Bit
		case LOW: *this.Port.GPIODAT &^= this.Bit
	}
}

func (this *GPIO)GPIOFGetData(pin uint8) bool{
	portData := this.Port.GetData()
	pinData := portData & (1<<pin)
	
	if pinData > 0 {
		return true
	} else {
		return false
	}
}




/*
func (this *GPIO) SetFun(FUNx uint8) {
	switch (this.Pin) {
		case 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15:
			*this.Port.ALTFN[0] &^= (0x3 << (this.Pin * 2))
			*this.Port.ALTFN[0] |= uint32(FUNx << (this.Pin * 2))
		default:
			*this.Port.ALTFN[1] &^= (0x3 << ((this.Pin - 16) * 2))
			*this.Port.ALTFN[1] |= uint32(FUNx << ((this.Pin - 16) * 2))
	}
}

func (this *GPIO) SetDir(IO uint8) {
	switch (IO) {
		case INPUT: *this.Port.OUTENB &^= this.Bit
		case OUTPUT: *this.Port.OUTENB |= this.Bit
	}
}


func (this *GPIO) SetDrv() {
	
}

func (this *GPIO) SetPull(Pull uint8) {
	switch (Pull) {
		case PULLDOWN:
			*this.Port.PULLENB &^= this.Bit
			*this.Port.PULLDEF |= this.Bit
		case PULLUP:
			*this.Port.PULLENB |= this.Bit
			*this.Port.PULLDEF |= this.Bit
		case PULLDEFAULT:
			*this.Port.PULLENB &^= this.Bit
	}
}

func (this *GPIO) SetSlew(Slew bool) {
	
	*this.Port.SLEWDEF |= this.Bit
	switch (Slew) {
		case true: *this.Port.SLEW &^= this.Bit
		case false: *this.Port.SLEW |= this.Bit
	}
}

func (this *GPIO) GetSlew() bool {
	return	((*this.Port.SLEWDEF & this.Bit) == this.Bit) && ((*this.Port.SLEW & this.Bit) == 0)
}

func (this *GPIO) SetData(Data bool) {
	switch (Data) {
		case true: *this.Port.OUT |= this.Bit
		case false: *this.Port.OUT &^= this.Bit
	}
}

func (this *GPIO) GetData() bool {
	return (*this.Port.OUT & this.Bit) == this.Bit
}

func (this *GPIO) Flip() {
	*this.Port.GPIODAT ^= this.Bit
}
*/
