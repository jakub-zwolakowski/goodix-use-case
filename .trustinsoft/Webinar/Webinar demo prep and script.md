# Webinarrr

## Prepare

* Prepare the editor and the code
  * Open the editor
  * Comment the IRQ scenario in function `main`.
  * Uncomment the `proc`-file scenario in function `main`.
  * Comment the line setting `addr_len` to `10` (keep it abstract) in function `tis_proc_write`.
  * Move the editor's view to show the function `main`.
* Start the analysis
  * Open a terminal.
  * Go to the root of the project.
  * `tis_choose`
  * `tis-analyzer --interpreter -tis-config-load .trustinsoft/config.json -tis-config-select 1 -gui`
* Prepare the GUI
  * Open the GUI in the browser.
  * Clean the GUI - close all function tabs and source code tabs.
  * Choose the light theme.
  * Click on the UB.

## Useful reading

* [The Goodix chip](https://www.goodix.com/en/about_goodix/newsroom/company_news/detail/1460)
* [Linux Device Drivers, Third Edition](https://lwn.net/Kernel/LDD3/)
  * Chapters 1, 2, 3 for basics.
  * Chapter 10 for interrupts.

### Some topics

* Kernel work queue
* Linux device files
* Linux proc files
* GPIO
* Device Tree
* IRQ
* I2C

## Video

[Demo during the Webinar Nov 2021](https://youtu.be/vRCNAj3pEYw?t=891)

## Script

### About the Goodix chip

* GT915 is (quote):
  * 5-point capacitive touch chip
  * specially designed for 4.5”~6” (four-and-half inch to six inch) medium and large sized mobile phones.
* Is used (or was used) in products from brands Huawei, Lenovo, ZTE, and others.
* This particular photo is the Goodix chip inside a the Wiko Rainbow 4G phone.

### The Code - what did we analyze?

* The driver's code was taken from this repository, the specific commit number is given here for reference.
* The product's official data-sheet was used to model the hardware behavior.
  * Unfortunately (or maybe fortunately for you?) it's in Chinese...
* I will not discuss the way that the analysis was prepared and set up.
  * We don't have here to enter in this level of details.
  * However - there is a report explaining all the details.

### Two ways to access the driver

* From the hardware side (hardware interrupts)
  * The *natural* scenario - we can access the touchscreen.
  * E.g. an attacker can simulate touching the touchscreen with 255 fingers.
    * (What Vic mentioned before.)
* Just for fun - access from the other side:
  * We can communicate with the driver using it's `proc` file.
  * So let's see what happens if the attacker has access from the OS side - can access the `proc` file.
  * This may be impossible in the actual context.
    * Maybe it would require at least root privileges on the OS or something like that.
    * But what if it was possible - what if we assume the attacker has access to this file and can write anything there?
      * In this case we've found a vulnerability!
    * So let's start with this scenario.

### Example 1

* Let us see the bug!
* SWITCH TO THE GUI
* The Analyzer shows us that something fishy is happening here.
* Disclaimer: THERE IS A LOT TO TAKE IN HERE!
  * This is not simple! This is not some toy example! This is code of an actual Linux driver!
* Let's see what is going on here:
  * Right pane - the original source code
    * Look up to the function name.
  * Middle pane - equivalent, interactive source code that we can explore.
    * With added information.
      * Explain red = dead code.
      * Explain the assertion = the Analyzer communicates with us.
        * Not like other tools, instead of just looking for bugs tries to prove there are no UBs.
        * Etc.
  * STEP BACK: Left pane - more information about the analysis.
    * Most important - the line saying that there is an UB.
  * Bottom pane - we can see the values of variables.
* Click on the `addr` field.
  * An array of two cells.
  * This is the TOP value - an abstract value - it can have any value in this particular type.
  * And what is that type?
    * Click on the type `u8` - it's `unsigned int`.
  * So: an array of two cells with values from 0 to 255.
* Look on the `addr_len` field.
  * TOP again.
  * And again `u8`.
  * So 0 to 255.
  * So, unless this is 0 or 1, we have indeed invalid memory read!
  * Seems very dangerous indeed!
* BTW, just for comparison look at another value, a precise one:
  * `data_len` : before the assigment = 0, after = 256.
  * You can use the Analyzer as a debugger - go forward and backward in the program's execution
* Now - click on `addr_len` - Show Defs.
  * All the statements that have influenced the value of this memory location at this point of execution.
  * Look - only one "direct":
    * NOTE: Indirects = on this line somewhere down the callstack there may be a direct assignment.
  * Click!
  * Wow!
    * We're in the function `__copy_from_user`!
    * So this value is taken from the userspace!
    * Seems dangerous indeed.
  * Look at `memcpy` arguments:
    * Copied TO `cmd_head` - in the kernel zone.
    * Look on the bottom pane.
      * Click to expand the values of the whole structure.
      * Compare before and after.
        * Before: field `addr_len` = 2, which would be OK.
        * After: field `addr_len` is abstract!
        * This is where the problem is introduced.
    * Copied FROM = this variable.
* Go back to caller - and caller - and caller...
  * Look - this is our function simulating the write to the `proc` file!
  * We create this variable and render it abstract.
    * Notice - this the same variable that we copied from before!
    * `tis_make_unknown` - function used to render data abstract.
  * We fill some fields, other field stay abstract.
  * What abstract means?
    * Not random, really any possible value considered in the same time.
  * And we can push this whole HUGE set of combinations of values through the program's execution like if it was a concrete value.
    * E.g. here we are calling a function passing it as an argument.
    * So this corresponds to performing a HUGE number of tests.
* Now let's see go to the source file
  * Entry point for the analysis = `main`
    * we initialize the driver,
    * we probe the hardware,
    * now - the analysis scenario.
      * Scenario 1 is commented, we start with scenario 2.
* `tis_proc_write`
  * Simulate writing to the `proc` file.
  * We prepare input = abstract.
  * OK, let's skip a little ahead - if we put a concrete value here - 10.
    * Uncomment the line setting `addr_len` to 10.
  * This is basically a proof that it will happen!
  * Relaunch the analysis. Go to the UB.
    * No we got NEVER TERMINATING (two bars on the right in the GUI) - it's sure that this UB will happen!
  * So - this might be an actual vulnerability.
  * However, you probably need privileges and access to write to the particular `proc` file.

### Example 2

* Another scenario - where the attacker controls access the to hardware.
  * This is the more *natural* scenario.
  * Implementation - we call the interrupt handlers in a loop.
  * Everything all right!
    * Lauch the analysis:
      * Left pane - "No undefined behaviors"
    * Go through the calls into the actual handler.
    * Find where the variable `fingers` is assigned.
    * Yes, we really check any number of fingers!
  * BTW, the loop is infinite.
    * No matter how many times this happens - everything is fine.
    * This is thanks to the mathematical nature of the tool.
      * TISA can find the loop's fixpoint and conclude that no matter how many times it is iterated, there is no UB.
  * This result is a big deal!
    * This is an actual proof that in this perimeter of analysis the software is immune to Undefined Behavior.

### Conclusion

This is the kind of results that TrustInSoft can give you:

* find subtle vulnerabilities,
* or prove absence of Undefined Behaviors in the source code in given perimeter of analysis.

### FAQ

* What about configuration, stubbing, set-up?
  * I cannot say it was very quick and easy in this case!
    * But I can say that it's easy for people who develop Linux drivers and are used to the Linux kernel's API (and can read a device's datasheet in Chinese) ;)
    * So it's relative to the difficulty of the software - and this is a Linux kernel driver!
    * There is a report with much more precise an complete info if you are curious.
* What is the size of all stubs and modelization?
  * Full size of all stubs ~600 lines
  * Real modelization that does anything is ~100 lines.
  * The rest is:
    * boilerplate code (e.g. function stubs that do nothing and return `0`),
    * and stuctures initialized with data from the data-sheet (i.e. copy-paste from the data-sheet).
* Do you analyze data races?
  * We could, but it was out of our scope here - this analysis is sequental.