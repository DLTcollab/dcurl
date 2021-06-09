package pow

/*
#cgo LDFLAGS: -ldcurl
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
extern bool dcurl_init(void *config);
extern void dcurl_destroy();
extern int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads);
bool dcurl_init_wrapper(){
	return dcurl_init(NULL);
}
char *dcurl_wrapper(char* trytes, int mwm, int threads){
	return (char*) dcurl_entry((int8_t*) trytes, mwm, threads);
}
*/
import "C"
import (
	"log"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"unsafe"

	. "github.com/iotaledger/iota.go/consts"
	. "github.com/iotaledger/iota.go/trinary"
	"github.com/pkg/errors"
)

type ProofOfWorkFunc = func(trytes Trytes, mwm int, parallelism ...int) (Trytes, error)

var (
	proofOfWorkFuncs = make(map[string]ProofOfWorkFunc)
)

func init() {
	C.dcurl_init_wrapper()
	proofOfWorkFuncs["Dcurl"] = DcurlProofOfWork
	proofOfWorkFuncs["SyncDcurl"] = SyncDcurlProofOfWork

	logger := log.New(os.Stdout, "WARN", log.Ldate|log.Ltime)
	gracefulShutdown := make(chan os.Signal)
	signal.Notify(gracefulShutdown, syscall.SIGTERM, syscall.SIGINT)
	go func() {
		<-gracefulShutdown
		logger.Println("Received shutdown request: Destorying dcurl")
		DcurlDestroy()
	}()
}

func DcurlDestroy() {
	C.dcurl_destroy()
}

func DcurlProofOfWork(trytes Trytes, mwm int, parallelism ...int) (Trytes, error) {
	return DcurlEntry(trytes, mwm, parallelism...)
}

var DcurlEntryMutex = sync.Mutex{}

func SyncDcurlProofOfWork(trytes Trytes, mwm int, parallelism ...int) (Trytes, error) {
	DcurlEntryMutex.Lock()
	defer DcurlEntryMutex.Unlock()

	nonce, err := DcurlEntry(trytes, mwm, parallelism...)
	if err != nil {
		return "", err
	}
	return nonce, nil
}

func DcurlEntry(trytes Trytes, mwm int, parallelism ...int) (Trytes, error) {
	if trytes == "" {
		return "", errors.New("invalid trytes supplied to Proof-of-Work func")
	}

	var numThread int
	if len(parallelism) != 0 && parallelism[0] > 0 {
		numThread = parallelism[0]
	} else {
		numThread = 1
	}

	returnTrytesC := C.dcurl_wrapper(C.CString(trytes), C.int(mwm), C.int(numThread))
	returnTrytesGO := C.GoString(returnTrytesC)[:TransactionTrytesSize]
	C.free(unsafe.Pointer(returnTrytesC))
	// Seperate the nonce from the end of the trytes
	nonce := returnTrytesGO[len(returnTrytesGO)-NonceTrinarySize/3 : len(returnTrytesGO)]

	return nonce, nil
}
