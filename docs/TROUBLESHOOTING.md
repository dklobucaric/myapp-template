
## Built-in JSON resources cannot be loaded

The application configuration is embedded through the Qt resource system. If the
application reports that `:/defaults/app-profile.json` or an environment profile
cannot be loaded, rebuild the project from a clean build directory. The resource
collection is explicitly initialized by `ConfigManager` before any built-in JSON
is read.
