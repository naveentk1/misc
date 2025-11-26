fn change(globalState: &mut u32)
{
    *globalState += 1;
}

fn main() {
    let mut globalState = 0u32;
    change(&mut globalState);
    println!("global_State: {}", globalState);
}