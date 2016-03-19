# Overview

LaunchGrid is an application launcher primarily intended to be used by software developers and QA specialists. It allows quickly launching application with various options. The launcher is highly configurable and allows representing a multitude of app shortcuts in a compact manner with a table.

The launcher allows a user to specify command lines for several application. Each command line can have a number of variables that are expanded based on provided flags. Each flag can be an set of predefined values, a directory or a file. 
For example if we want to execute `dir` command (`cmd /k dir`) and want to choose either detailed or list view (`/w`), we'd specify an app with command line `cmd` and command line arguments: `/k dir $(verbosity)`. We also need to specify a flag named "verbosity" with two choices: "detail" with empty value and "list" with value `/w`. Setting this up will create a launch table:

|         | detail |  list  |
| -------:|:------:|:------:|
| **dir** |  Run   |  Run   |

Now when a user clicks one of the "Run" links the launcher will launch `cmd` with chosen arguments.
Now if we want to add an optional filter to show everything, files only or directories only, we need to add another flag, say named "filter" with choices: "all" with empty value, "files" with value `/a-d` and "directories" with value `/ad`. We also need to add `$(filter)` to the dir command line arguments. This will generate a table:

|         |     all    |            |    files   |            | directories |            |
| -------:|:----------:| ----------:|:----------:| ----------:|:-----------:| ----------:|
|         | **detail** |  **list**  | **detail** |  **list**  | **detail**  |  **list**  |
| **dir** |     Run    |     Run    |     Run    |     Run    |     Run     |     Run    |

Now say we want to run dir command in one of the sub-directories of `c:\myproject`. We can add a directory flag named "directory" with value `c:\myproject\*` and name `$(FileName:directory)`. This name would be expanded to the name of the directory. We also change dir application arguments to `/k dir $(directory) $(verbosity) $(filter)`. If the number of sub-directories is large we can change directory option to be presented as a combo box.

# Settings
## General tab
You can use LaunchGrid as an auto-close window, when the launcher will exit as soon as its window looses focus, or as a background "pinned to desktop" window when the launcher window appears on the desktop and has no icon on the taskbar. In the latter mode the launcher can auto-start when the user logs in ("Automatically start the program when user logs in" check box). When in auto-close mode the launcher can start applications in the background so that they don't steal focus and the launcher window can stay opened ("Do not close the window when program is launched" check box). This may be useful if you are planning to launch multiple applications at once.

The theme option allows changing launcher colors between light and dark.

## Content tab
This is where you set up your programs and options.

The "Tabs" group allows you to specify multiple unrelated tabs with their own set of applications and options. 

The "Flags" group define flags for your applications. The "App" flag is the special flag that defines you applications. You can add new flags of different types. The "Flag" type allows you to define an explicit set of possible values. "Directory" and "File" flags allows enumerating directories and files using wildcards.
You can specify flag position in resulting table to be in rows, columns or moved out of the table to a separate combo box (the "Select" option).

The "Flag Values" group is where you define values for the selected flag. Controls here depend on the type of flag selected.

For App flag you need to specify the Name that appears in the launcher table, Command that contains a path to the executable or a file to open, program arguments in Arguments field, and optionally a DDS verb that is used to open the file or execute the application. 

Both Command and Arguments can contain references to other flags. The syntax for flag value is `$(name)` where `name` is the name of the flag. You can also use a number of transforms to a flag value:

<dl>
  <dt>$(FileName:name)</dt><dd>extracts file name from the flag value</dd>
  <dt>$(Directory:name)</dt><dd>extracts directory path from the flag value
  <dt>$(Extension:name)</dt><dd>extracts file extension from the flag value
  <dt>$(Path:begin:end :name)</dt><dd>extracts path of the path from the flag value. The value is broken into components separated by backslash and the sub-portion of those values starting with indices from begin to end inclusive are returned. Both begin and end can be negative in which case they refer to the end of the list of components.</dd>
</dl>

For Flag type you specify possible flag values as Name, Value pairs. Names appear in the launcher window, while Values are used for substitution.

For Directory and File types Value specifies the path to directory or file possibly including asterisk. The name can contain a variable reference like Command or Arguments for apps, but the only variable it recognizes is the selected flag. This allows names for values for these types of flags to contain found paths. 
Menu
Menu tab allows adding custom menu items into LaunchGrid main menu. Each menu item is specified similarly to an App in Content tab. The Path can contain asterisk like a File flag and similarly to File flag, the Name can  contain variable referencing the item's Path, the variable is `$(MenuItem)`. You can also use variables for flags that are in "Select" position, i.e. rendered as combo boxes.

# Building
Building the project requires Visual Studio 2015. Building the installer also requires WIX.
To build the project, open the solution in Visual Studio and build or use MSBuild from command line.
