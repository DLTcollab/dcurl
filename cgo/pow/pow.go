package pow

/*
#cgo LDFLAGS: -ldcurl

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
extern bool dcurl_init();
extern bool dcurl_destroy();
extern int8_t *dcurl_entry(int8_t *trytes, int mwm, int threads);

char *dcurl_wrapper(char* trytes, int mwm, int threads){
	return (char*) dcurl_entry((int8_t*) trytes, mwm, threads);
}
*/
import "C"
import (
	"sync"

	. "github.com/iotaledger/iota.go/consts"
	. "github.com/iotaledger/iota.go/trinary"
	"github.com/pkg/errors"
)

type ProofOfWorkFunc = func(trytes Trytes, mwm int, parallelism ...int) (Trytes, error)

var (
	proofOfWorkFuncs = make(map[string]ProofOfWorkFunc)
)

func init() {
	proofOfWorkFuncs["Dcurl"] = DcurlProofOfWork
	proofOfWorkFuncs["SyncDcurl"] = SyncDcurlProofOfWork
}

func DcurlProofOfWork(trytes Trytes, mwm int, parallelism ...int) (Trytes, error) {
	C.dcurl_init()
	defer C.dcurl_destroy()

	return DcurlEntry(trytes, mwm, parallelism...)
}

var DcurlEntryMutex = sync.Mutex{}

func SyncDcurlProofOfWork(trytes Trytes, mwm int, parallelism ...int) (Trytes, error) {
	C.dcurl_init()
	defer C.dcurl_destroy()

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
	returnTrytesGO := C.GoString(returnTrytesC)
	nonce := returnTrytesGO[len(returnTrytesGO)-NonceTrinarySize/3-1 : len(returnTrytesGO)-1]

	return nonce, nil
}
