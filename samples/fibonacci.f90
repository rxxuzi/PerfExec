program fibonacci
    implicit none
    integer :: n

    n = 35
    print *, fibo(n)

contains

    recursive function fibo(n) result(x)
        integer :: n
        integer :: x  ! 戻り値

        if (n <= 1) then
            x = n
        else
            x = fibo(n-1) + fibo(n-2)
        end if
    end function fibo

end program fibonacci

