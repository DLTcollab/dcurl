package pow

import (
	"sync"
	"testing"

	. "github.com/iotaledger/iota.go/consts"
	"github.com/iotaledger/iota.go/curl"
	"github.com/iotaledger/iota.go/guards"
	"github.com/stretchr/testify/assert"
)

var rawTx = "999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999A9RGRKVGWMWMKOLVMDFWJUHNUNYWZTJADGGPZGXNLERLXYWJE9WQHWWBMCPZMVVMJUMWWBLZLNMLDCGDJ999999999999999999999999999999999999999999999999999999YGYQIVD99999999999999999999TXEFLKNPJRBYZPORHZU9CEMFIFVVQBUSTDGSJCZMBTZCDTTJVUFPTCCVHHORPMGCURKTH9VGJIXUQJVHK999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999"
var mwm = 14

func TestDcurlProofOfWork(t *testing.T) {
	assert := assert.New(t)
	t.Log("Use dcurl to calculate PoW with 8 threads enabled and MWM = 14")
	nonce, err := DcurlProofOfWork(rawTx, mwm, 8)
	assert.Nil(err)
	assert.Equal(len(nonce), 27)

	rawTx = rawTx[:len(rawTx)-NonceTrinarySize/3] + nonce
	hashedTrytes := curl.MustHashTrytes(rawTx)

	t.Log("Hashed trytes:", hashedTrytes)
	assert.True(guards.IsTransactionHashWithMWM(hashedTrytes, uint(mwm)))

}

func TestSyncDcurlProofOfWork(t *testing.T) {
	waitingGroup := new(sync.WaitGroup)
	waitingGroup.Add(10)

	assert := assert.New(t)
	t.Log("Use Sync dcurl to calculate 10 requests of PoW with 8 threads enabled and MWM = 14")
	for i := 0; i < 10; i++ {
		go func(index int) {
			nonce, err := SyncDcurlProofOfWork(rawTx, mwm, 8)
			assert.Nil(err)
			assert.Equal(len(nonce), 27)
			returnTx := rawTx[:len(rawTx)-NonceTrinarySize/3] + nonce
			hashedTrytes := curl.MustHashTrytes(returnTx)
			t.Log("The", index+1, "th hashed trytes:", hashedTrytes)
			assert.True(guards.IsTransactionHashWithMWM(hashedTrytes, uint(mwm)))
			waitingGroup.Done()
		}(i)
	}
	waitingGroup.Wait()
}