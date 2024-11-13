# auction-network

This is final project for SoICT Network Programming Lab.

## Project structure

```sh
.
├── app
│   ├── client.c
│   ├── server.c
│   └── users.txt
├── GUI
│   ├── gui
│   └── gui.c
├── model
│   ├── message.c
│   ├── message.h
│   ├── user.c
│   └── user.h
├── README.MD
```

**To run**
```sh
cd release
make
```

Compile GUI:
```c
gcc gui.c -o gui `pkg-config --cflags --libs gtk4`
```

`pkg-config --cflags --libs gtk4`:
- `pkg-config`: Dùng để tìm kiếm và lấy các thông tin cần thiết về thư viện GTK (các thông tin cờ biên dịch và đường dẫn thư viện).
- `--cflags`: Lấy các cờ biên dịch cần thiết.
- `--libs`: Lấy các đường dẫn liên kết tới các thư viện cần thiết cho GTK.
gtk4: Phiên bản của GTK mà bạn đang sử dụng.

