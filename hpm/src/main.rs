mod initer;
mod installer;
mod download_package;

use std::env;
use clap::{App, Arg};

fn main() {
    let matches = App::new("HPM")
        .about("Hyperion Package Manager")
        .version("1.0")
        .subcommand(
            App::new("init")
                .about("Initialize a project.")
        )
        .subcommand(
            App::new("install")
                .about("Install a package.")
                .arg(
                    Arg::with_name("name")
                        .help("Package name")
                        .required(true)
                        .index(1),
                ),
        )
        .get_matches();

    if let Some(_) = matches.subcommand_matches("init") {
        initer::init_hypl();
    } else if let Some(matches) = matches.subcommand_matches("install") {
        let name = matches.value_of("name").unwrap();
        installer::install_package(name.to_string());
    } else {
        println!("{}", matches.usage());
    }
}

