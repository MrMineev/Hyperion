mod initer;
mod installer;
mod download_package;

use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();

    if args[1].to_string() == "init" {
        initer::init_hypl();
    } else if args[1].to_string() == "install" {
        installer::install_package();
        // INSTALL MODULE
    }
    println!("{:?}", args);
}

