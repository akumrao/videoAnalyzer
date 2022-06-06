
https://www.itu.int/rec/T-REC-H.264-200503-S/en

for i in *.htm; do mv $i "${i%%__*}"; done


x264EncdoerDecoder -x -c  ./test.264




NAL_SLICE_DPA   = 2,
NAL_SLICE_DPB   = 3,
The coded data that makes up a slice is placed in three separate Data Partitions (A, B and C), each containing a subset of the coded slice. Partition A contains the slice header and header data for each macroblock in the slice, Partition B contains coded residual data for Intra and SI slice macroblocks and Partition C contains coded residual data for inter coded macroblocks (forward and bi-directional). Each Partition can be placed in a separate NAL unit and may therefore be transported separately.

If Partition A data is lost, it is likely to be difﬁcult or impossible to reconstruct the slice, hence Partition A is highly sensitive to transmission errors. Partitions B and C can (with careful choice of coding parameters) be made to be independently decodeable and so a decoder may (for example) decode A and B only, or A and C only, lending ﬂexibility in an error-prone environment.