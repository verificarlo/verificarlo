def test_function(int x):
    cdef double result
    result = 1.4
    for i in range(x):
        result += 0.1
    print(result)
