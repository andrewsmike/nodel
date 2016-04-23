# Fibo.asm: Print the first arg1 numbers in the fibonacci sequence.
# Prints, adds one number to the other, then swaps while count > 0.

copy arg1 -> count

copy 0 -> a
copy 1 -> b

branch count, 0 | gt=:base
exit

base:
 print b
 add a, b -> a

 copy a -> c # SWAP(a, b)
 copy b -> a
 copy c -> b

 sub count, 1 -> count

branch count, 0 | gt=:base

exit
