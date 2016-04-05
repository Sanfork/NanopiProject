package main

import (
	_"io"
	"log"
	"net/http"
	"html/template"
	"fmt"
	"nanopi/S3C2451"
)

var PF2 *S3C2451.GPIO
type LedStatus struct{
	Status string
}

func switchHandler(w http.ResponseWriter, r *http.Request) {
	s := new(LedStatus)
	status := PF2.GPIOFGetData(PF2.Pin)

	if r.Method == "GET" {  /*GET method explain*/
		
		if status {
			s.Status = "ON"
		}else {
			s.Status = "OFF"
		}
		
		t, err := template.ParseFiles("switch.html")
		if err != nil {
			http.Error(w, err.Error(),http.StatusInternalServerError)
			return
		}
			t.Execute(w, s)
			return
	} 
	
	if r.Method == "POST" {
		str := r.FormValue("switch")
		fmt.Println(str)
		if str == "Switch" {
			if status {
				s.Status = "OFF"                  /*change information*/
				PF2.GPIOFSetData(S3C2451.LOW)     /*Switch LED to OFF*/
			} else {
				s.Status = "ON"
				PF2.GPIOFSetData(S3C2451.HIGH)    /*Switch LED to ON*/
			}

		} 
	
		t, err := template.ParseFiles("switch.html")
		if err != nil {
			http.Error(w, err.Error(),http.StatusInternalServerError)
			return
		}
			t.Execute(w, s)
		
		return
	} 

}

func main() {
	/*initialize GPIO*/
	
	defer S3C2451.FreeS3C2451()
	PF2, _ = S3C2451.CreateGPIO(S3C2451.PF, 2)
	defer S3C2451.FreeGPIO(PF2)
	PF2.GPIOFSetDir(S3C2451.OUTPUT)
	


	http.HandleFunc("/switch", switchHandler)
	err := http.ListenAndServe(":9090", nil)
	if err != nil {
		log.Fatal("ListenAndServe: ", err.Error())
	}
}