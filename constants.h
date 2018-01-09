#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define MinTryteValue -13
#define MaxTryteValue 13
#define SignatureSize 6561
#define HashSize 243
#define Depth 3
#define Radix 3

#define SignatureMessageFragmentTrinaryOffset 0
#define SignatureMessageFragmentTrinarySize 6561
#define AddressTrinaryOffset SignatureMessageFragmentTrinaryOffset + SignatureMessageFragmentTrinarySize
#define AddressTrinarySize 243
#define ValueTrinaryOffset AddressTrinaryOffset + AddressTrinarySize
#define ValueTrinarySize 81
#define ObsoleteTagTrinaryOffset ValueTrinaryOffset + ValueTrinarySize
#define ObsoleteTagTrinarySize 81
#define TimestampTrinaryOffset ObsoleteTagTrinaryOffset + ObsoleteTagTrinarySize
#define TimestampTrinarySize 27
#define CurrentIndexTrinaryOffset TimestampTrinaryOffset + TimestampTrinarySize
#define CurrentIndexTrinarySize 27
#define LastIndexTrinaryOffset CurrentIndexTrinaryOffset + CurrentIndexTrinarySize
#define LastIndexTrinarySize 27
#define BundleTrinaryOffset LastIndexTrinaryOffset + LastIndexTrinarySize
#define BundleTrinarySize 243
#define TrunkTransactionTrinaryOffset BundleTrinaryOffset + BundleTrinarySize
#define TrunkTransactionTrinarySize 243
#define BranchTransactionTrinaryOffset TrunkTransactionTrinaryOffset + TrunkTransactionTrinarySize
#define BranchTransactionTrinarySize 243
#define TagTrinaryOffset BranchTransactionTrinaryOffset + BranchTransactionTrinarySize
#define TagTrinarySize 81
#define AttachmentTimestampTrinaryOffset TagTrinaryOffset + TagTrinarySize
#define AttachmentTimestampTrinarySize 27

#define AttachmentTimestampLowerBoundTrinaryOffset AttachmentTimestampTrinaryOffset + AttachmentTimestampTrinarySize
#define AttachmentTimestampLowerBoundTrinarySize 27
#define AttachmentTimestampUpperBoundTrinaryOffset AttachmentTimestampLowerBoundTrinaryOffset + AttachmentTimestampLowerBoundTrinarySize
#define AttachmentTimestampUpperBoundTrinarySize 27
#define NonceTrinaryOffset AttachmentTimestampUpperBoundTrinaryOffset + AttachmentTimestampUpperBoundTrinarySize
#define NonceTrinarySize 81

#define transactionTrinarySize SignatureMessageFragmentTrinarySize + AddressTrinarySize + ValueTrinarySize + ObsoleteTagTrinarySize + TimestampTrinarySize + CurrentIndexTrinarySize + LastIndexTrinarySize + BundleTrinarySize + TrunkTransactionTrinarySize + BranchTransactionTrinarySize + TagTrinarySize + AttachmentTimestampTrinarySize + AttachmentTimestampLowerBoundTrinarySize + AttachmentTimestampUpperBoundTrinarySize + NonceTrinarySize

extern char TryteAlphabet[];

#endif
