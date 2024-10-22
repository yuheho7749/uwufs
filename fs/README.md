## How to Build and Run
1. Clone the repository
2. `mkdir fs/build && cd $_`
3. `cmake ..`
4. `make`
5. `sudo ./fs`

## Pitfalls
1. It is unclear whether directly reading fields in a block or reading the whole block and extracting fields is more efficient.