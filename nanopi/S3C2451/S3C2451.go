/*
	Author: Hiram
*/

package S3C2451

import (
	"os"
	"unsafe"
	"syscall"
)

var iS3c2451 *S3C2451 = nil

func IS3C2451() (*S3C2451) {
	if (iS3c2451 == nil) {
		iS3c2451 = &S3C2451{nil}
		iS3c2451.hFile, _ = os.OpenFile("/dev/mem", os.O_RDWR, 0)
	}

	return iS3c2451
}

func FreeS3C2451() {
	if iS3c2451 != nil { 
		iS3c2451.hFile.Close()	
		iS3c2451 = nil	
	}
}

/*****************************************************************************/

type S3C2451 struct {
	hFile		*os.File
	
	//epollFD		int
	//epollEvent	map[uint32]func()
}

func (this *S3C2451) GetMMap(BaseAddr uint32) ([]uint8, bool) {
	if this.hFile == nil { return nil, false }

	mem, err := syscall.Mmap(int(this.hFile.Fd()), int64(BaseAddr & 0xFFFFF000), 
		os.Getpagesize(), syscall.PROT_READ | syscall.PROT_WRITE, syscall.MAP_SHARED)
		
	return mem, (err == nil)
}

func (this *S3C2451) FreeMMap(hMem []uint8) {
	syscall.Munmap(hMem)
}

func (this *S3C2451) Register(hMem []uint8, offset uint32) (*uint32, bool) {
	if hMem == nil { return nil, false }
	return  (*uint32)(unsafe.Pointer(&hMem[offset])), true
}

/*****************************************************************************/
/*func (this *A8x) setupEpoll() {
	var err error
	this.epollEvent = make(map[uint32] chan bool)
	
	this.epollFD, err = syscall.EpollCreate1(0)
	if err != nil {
		fmt.Println("CreateEpoll Error")
		return
	}
	
	go func () {
		var epollEvents [this.epollEvent.Count]syscall.EpollEvent
		for {
			Len, err := syscall.EpollWait(this.epollFD, epollEvents[ : ], -1)
			if err != nil {
				if err == syscall.EAGAIN { continue }
				fmt.Println("EpollWait Error")
				return
			}
			
			for I := 0; I < Len; I++ {
				if X, ok := this.epollEvent[int(epollEvents[I].Fd)]; ok {
					X <- true
				}
			}
		}
	}()
}

func (this *A8x) AddISR(RegAddr uint32, Interrupt chan bool) (error) {
	var event syscall.EpollEvent
	event.Events = syscall.EPOLLIN | (syscall.EPOLLET & 0xFFFFFFFF) | syscall.EPOLLPRI
	
	this.epollEvent[RegAddr] = Interrupt
	err := syscall.SetNonblock(Interrupt, true)
	if err != nil { return err }
	
	event.Fd = int32(RegAddr)
	err = syscall.EpollCtl(this.epollFD, syscall.EPOLL_CTL_ADD, RegAddr, &event)
	if err != nil { return err }
	
	return nil
}

func (this *A8x) DelISR(RegAddr uint32) (error) {
	err := syscall.EpollCtl(this.epollFD, syscall.EPOLL_CTL_DEL, RegAddr, nil)
	if err != nil { return err }
	
	err = syscall.SetNonblock(RegAddr, false)
	if err != nil { return err }
	
	delete(this.epollEvent, RegAddr)
	return nil
}*/

