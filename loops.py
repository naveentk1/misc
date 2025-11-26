def binary_search(data_array, element):

    low = 0
    high = len(data_array) - 1
    
    while low <= high:
        mid = (low + high) // 2  # Use // for integer division
        guess = data_array[mid]
        
        if guess == element:
            print(f"Element found at {mid}th index")
            return
        elif guess > element:
            high = mid - 1
        else:
            low = mid + 1
    
    print("Element Not Found")
    return
    
if __name__ == "__main__":
    data_array = [2, 10, 23, 44, 100, 121]
    
    binary_search(data_array, 3)    # not found case
    binary_search(data_array, 2)    # found at corner case
    binary_search(data_array, 44)   # found at middle case