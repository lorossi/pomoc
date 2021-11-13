# Pomoc

While I should be studying, I am developing a little pomodoro timer to help me study.

That's ironic because it's taking me a lot of time.
I mean, you have to spend money to make money, but spending time to make time does not sound good.
I don't even know what to do with my life any more.

Basically I am using my own [library](https://github.com/lorossi/cli-gui) to handle all the windows in the terminal.

More on this project to come.

## Custom time settings

Custom time settings can be provided to the pomodoro timer by using arguments. In order, you can provide:

1. the duration of a study session
2. the duration of a short break
3. the duration of a long break
4. the number of study sessions before a long break

Up until 4 parameters can be passed, while the remaining ones will keep the default values (45 minutes study sessions, 15 and 30 minutes for respectively short and long break, 4 study sessions before a long break).

The parameters will be saved into a save file, so that the next time the program will retain them. In order to reset them to default, call the program by passing `reset` as the only parameter.

## License

This project is distributed under Attribution 4.0 International (CC BY 4.0) license.
