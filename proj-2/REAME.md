# FTP Code Instructions

## How to Run

To compile and execute the program, follow these steps:

1. Run the following command to generate the `download` binary:
   ```bash
   make
   ```
2. After the binary is created, you can test the program by running:
    ```bash
    ./download ftp://[<user>:<password>@]<host>/<url-path>
    ```
Replace `<user>`, `<password>`, `<host>`, and `<url-path`> with the appropriate values for the FTP server you want to access.
