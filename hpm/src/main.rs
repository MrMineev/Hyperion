use std::env;

fn main() {
    let args: Vec<String> = env::args().collect();
    
    if (args[1].to_string() == "init") {
        // INIT PROJECT
    } else if (args[1].to_string() == "install") {
        // INSTALL MODULE
    }
    println!("{:?}", args);
}

