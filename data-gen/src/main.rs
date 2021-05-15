use std::fs::File;
use std::io::prelude::*;
use std::path::Path;
use std::env;

const MINS_1_DAY: u32 = 1440;
const HR:u32 = 60;
const START_DAY: u32 = 0;
const DAY_COUNT: u32 = 7;
const MAX_VALUES: u32 = 56;
const ORD_OF_ZERO: u32 = 48;

fn main() {
    let args: Vec<String> = env::args().collect(); 
    let mut times: Vec<u32> = Vec::with_capacity(10);
    let mut file_name: &str = "";
    for index in 1..args.len() {
        if args[index].starts_with("file:") {
            file_name = &args[index][5..];  
            println!("Data Gen: Output file: {}",file_name);
        } else {
            times.push(parse_time(&args[index]));
        }
    }
    if file_name == "" {
        panic!("file:<filename> argument was not specified");
    }
    if times.len() == 0 {
        panic!("No times were specified");
    }
    print!("Input Times: {} values [", times.len());
    for ti in times.iter() {
        print!("{} ",ti);
    }
    println!("]");
    create_values(times, file_name);
}

fn parse_time(s: &str) -> u32 {
    let mut h: u32 = 0;
    let mut m: u32 = 0;
    let mut hrs = true;
    for c in s.chars() {
        if (c == ':') || (c == '.') {
            hrs = false;
        } else {
            if c.is_numeric() {
                if hrs {
                    h = (h * 10) + ((c as u32) - ORD_OF_ZERO);
                } else {
                    m = (m * 10) + ((c as u32) - ORD_OF_ZERO);
                }
            } else {
                panic!("Invalid character: '{}' in Parameter: '{}'",c,s);
            }    
        }
    }
    if h > 23 {
        panic!("Hour is > 23: Parameter: '{}'",s);
    }
    if m > 59 {
        panic!("Minute is > 59: Parameter: '{}'",s);
    }
    println!("Param {} --> {}:{} ({}) mins",s,h,m, (h * 60) + m);
    return (h * 60) + m; 
}

fn create_values(day_values: Vec<u32>, filename: &str, ) {
    let mut out_str = String::with_capacity(50);
    let mut count:u32  = 0;
    out_str.push_str("[");
    for day in START_DAY..DAY_COUNT {
        for hr in day_values.iter() {
            let b: u32 = (day * MINS_1_DAY) + (HR * hr);
            out_str.push_str(format!("{},", b).as_str());
            count = count + 1;
            if count > MAX_VALUES {
                panic!("Too Many values. Count is {}. Max is {}. File is {}" ,count,MAX_VALUES, filename);
            }
        }
    } 
    out_str.truncate(out_str.len()-1);
    out_str.push_str("]");
    write_file(filename, out_str, count);
}

fn write_file(filename: &str, out_str: String, count: u32) {
    let path = Path::new(filename);
    let display = path.display();
    let mut file = match File::create(&path) {
        Err(why) => panic!("couldn't create {}: {}", display, why),
        Ok(file) => file,
    };
    file.write(out_str.as_bytes()).expect("Failed to write");
    println!("{} --> {} values {}", path.display(),count, out_str);
}