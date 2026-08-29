#include "../../src/injector/injector_api.hpp"
#include "../../src/payload/messages.cpp"
#include <iostream>

int wmain(int argc, wchar_t* argv[]) {
    bool verbose = true;
    int ret = 0;
    char* app_bound_encrypted_key="QVBQQgEAAADQjJ3fARXREYx6AMBPwpfrAQAAAEIFPa64usdIp89Z90A0TTEQAAAAHAAAAEcAbwBvAGcAbABlACAAQwBoAHIAbwBtAGUAAAAQZgAAAAEAACAAAABZWpeZnUMaCyvtPE8F05XH2gtbNl/iiVuXi6hZ/hTWRQAAAAAOgAAAAAIAACAAAAAdEw5TmyVCJofp2C41boXuqgADRXla1Vwy9QOjt/ml+pABAADUY9BwAllug/UGqFvS/bjIe8GWzqIfefMlIiq/dZtytkgnffhY0ZeAYx6taYQCr8yJ8I+LDFdVm39bZ/6dza5V5MVfqmpyJ1IrQh3UJUm1r5pvdF6d52SUOT6gGMhnmg+wc138yaxciwEz2MP6NRjTaAeV8tOWSBXrJIYx3nBudQKrKez2REUzttj43WcieOqTjS+GfrN9LE3KOkMNJKX5tk8xAKQAdXH9oqlkIganj7qEp8tqYv+qqhVUtRDKLfaCu94aFbR3hkUl14n4+DKq5+i+zFAk4xG9EInRbZA4bJd3sYWM2h3gFy1Pu6/WNaw23qHM0XHB/aoDHEzET3zDeiKXwyrFVCFX0Hufri8SEcvF9O81PI3KIp3lT3J5mRpeEF2X910uFY8eSL+NRzs8QPRAS2LsXRyCpn1wvG7a+1VbKyaxB90Si6KFyk6ovIfSseKwZxe+kQPF6FRpQ5hacogkUitSn/nhAqSH0d2TVJMTvFXgZHfYdFtL3PtxoPH7BVDDx6zvAQkKJWt2Y98tQAAAAIx7ZO7xAS+e00GJcpSSZ8mnShIuEEvJ4Pq6AkQYiznIbrLeAlTLmbZ5TrRQZ3XbyhEwOhSKRKVqhRdbOgDCIP0=";
    std::cout << "App Bound Encrypted Key: " << app_bound_encrypted_key << std::endl;
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    size_t outEncryptedKeyLength = 0;
    ret = Injector::Decrypt(0, L"", app_bound_encrypted_key, std::strlen(app_bound_encrypted_key), buffer, outEncryptedKeyLength);
    std::cout << "Decrypt returned: " << ret << std::endl;
    std::cout << "Output key length: " << outEncryptedKeyLength << std::endl;
    std::cout << "Output key: " << buffer << std::endl;
    return ret;
}