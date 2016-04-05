// +build

package main

import (
	"fmt"
	"log"
	"strings"

	"github.com/paypal/gatt"
	"github.com/paypal/gatt/examples/option"
	//"github.com/paypal/gatt/examples/service"
	"../nanopi/S3C2451"
)

var PF2 *S3C2451.GPIO

func NewNanoPiBLEDemoService() *gatt.Service {

	
	data := make([]byte, 256)
	s := gatt.NewService(gatt.MustParseUUID("09fc95c0-c111-11e3-9904-0002a5d5c51b"))
	s.AddCharacteristic(gatt.MustParseUUID("11fac9e0-c111-11e3-9246-0002a5d5c51b")).HandleReadFunc(
		func(rsp gatt.ResponseWriter, req *gatt.ReadRequest) {
			fmt.Printf("Send data to device: Hi, by NanoPi")
			fmt.Printf("\n")
			fmt.Fprintf(rsp, "Hi, by NanoPi")
		})

	c := s.AddCharacteristic(gatt.MustParseUUID("16fe0d80-c111-11e3-b8c8-0002a5d5c51b"))
	c.HandleWriteFunc(
		func(r gatt.Request, newData []byte) (status byte) {
			for i := 0; i < len(data) && i < len(newData); i++ {
				data[i] = newData[i]
			}
			fmt.Printf("Recv data from device: ")
			for i := 0; i < len(newData); i++ {
				fmt.Printf("%x ", newData[i])
			}
			fmt.Printf("\n")
			c := string(newData[:])
			fmt.Println(c)
			if(strings.Contains(c,"LedSwitch")) {
				//defer S3C2451.FreeS3C2451()
				//PF2, _ := S3C2451.CreateGPIO(S3C2451.PF, 2)
				//defer S3C2451.FreeGPIO(PF2)
				
				data := PF2.Port.GetData()
				if((data & (0x1 << 2))>0){
					PF2.GPIOFSetData(S3C2451.LOW)
				} else {
					PF2.GPIOFSetData(S3C2451.HIGH)
				}
			}

			return gatt.StatusSuccess
		})

	return s
}

func main() {
	/*initialize GPIO*/
	defer S3C2451.FreeS3C2451()
	PF2, _ = S3C2451.CreateGPIO(S3C2451.PF, 2)
	defer S3C2451.FreeGPIO(PF2)
	PF2.GPIOFSetDir(S3C2451.OUTPUT)
	
	/*Create BLE Device*/
	d, err := gatt.NewDevice(option.DefaultServerOptions...)
	if err != nil {
		log.Fatalf("Failed to open device, err: %s", err)
	}

	// Register optional handlers.
	d.Handle(
		gatt.CentralConnected(func(c gatt.Central) { fmt.Println("Connect: ", c.ID()) }),
		gatt.CentralDisconnected(func(c gatt.Central) { fmt.Println("Disconnect: ", c.ID()) }),
	)
	
	fmt.Println("NAnopi Start")
	// A mandatory handler for monitoring device state.
	onStateChanged := func(d gatt.Device, s gatt.State) {
		fmt.Printf("State: %s\n", s)
		switch s {
		case gatt.StatePoweredOn:
			s1 := NewNanoPiBLEDemoService()
			d.AddService(s1)

			// Advertise device name and service's UUIDs.
			d.AdvertiseNameAndServices("NanoPi", []gatt.UUID{s1.UUID()})
		default:
		}
	}

	d.Init(onStateChanged)
	select {}
}
