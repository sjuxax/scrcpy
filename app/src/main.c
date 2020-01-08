#include "scrcpy.h"

#include <stdbool.h>
#include <unistd.h>
#include <libavformat/avformat.h>
#define SDL_MAIN_HANDLED // avoid link error on Linux Windows Subsystem
#include <SDL2/SDL.h>

#include "config.h"
#include "cli.h"
#include "compat.h"
#include "util/log.h"
#include "recorder.h"

struct args {
    struct scrcpy_options opts;
    bool help;
    bool version;
};

static void usage(const char *arg0) {
#ifdef __APPLE__
# define CTRL_OR_CMD "Cmd"
#else
# define CTRL_OR_CMD "Ctrl"
#endif
    fprintf(stderr,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "\n"
        "    --always-on-top\n"
        "        Make scrcpy window always on top (above other windows).\n"
        "\n"
        "    -b, --bit-rate value\n"
        "        Encode the video at the given bit-rate, expressed in bits/s.\n"
        "        Unit suffixes are supported: 'K' (x1000) and 'M' (x1000000).\n"
        "        Default is %d.\n"
        "\n"
        "    --crop width:height:x:y\n"
        "        Crop the device screen on the server.\n"
        "        The values are expressed in the device natural orientation\n"
        "        (typically, portrait for a phone, landscape for a tablet).\n"
        "        Any --max-size value is computed on the cropped size.\n"
        "\n"
        "    -f, --fullscreen\n"
        "        Start in fullscreen.\n"
        "\n"
        "    -h, --help\n"
        "        Print this help.\n"
        "\n"
        "    --max-fps value\n"
        "        Limit the frame rate of screen capture (only supported on\n"
        "        devices with Android >= 10).\n"
        "\n"
        "    -m, --max-size value\n"
        "        Limit both the width and height of the video to value. The\n"
        "        other dimension is computed so that the device aspect-ratio\n"
        "        is preserved.\n"
        "        Default is %d%s.\n"
        "\n"
        "    -n, --no-control\n"
        "        Disable device control (mirror the device in read-only).\n"
        "\n"
        "    -N, --no-display\n"
        "        Do not display device (only when screen recording is\n"
        "        enabled).\n"
        "\n"
        "    -p, --port port\n"
        "        Set the TCP port the client listens on.\n"
        "        Default is %d.\n"
        "\n"
        "    --prefer-text\n"
        "        Inject alpha characters and space as text events instead of\n"
        "        key events.\n"
        "        This avoids issues when combining multiple keys to enter a\n"
        "        special character, but breaks the expected behavior of alpha\n"
        "        keys in games (typically WASD).\n"
        "\n"
        "    --push-target path\n"
        "        Set the target directory for pushing files to the device by\n"
        "        drag & drop. It is passed as-is to \"adb push\".\n"
        "        Default is \"/sdcard/\".\n"
        "\n"
        "    -r, --record file.mp4\n"
        "        Record screen to file.\n"
        "        The format is determined by the --record-format option if\n"
        "        set, or by the file extension (.mp4 or .mkv).\n"
        "\n"
        "    --record-format format\n"
        "        Force recording format (either mp4 or mkv).\n"
        "\n"
        "    --render-expired-frames\n"
        "        By default, to minimize latency, scrcpy always renders the\n"
        "        last available decoded frame, and drops any previous ones.\n"
        "        This flag forces to render all frames, at a cost of a\n"
        "        possible increased latency.\n"
        "\n"
        "    -s, --serial serial\n"
        "        The device serial number. Mandatory only if several devices\n"
        "        are connected to adb.\n"
        "\n"
        "    -S, --turn-screen-off\n"
        "        Turn the device screen off immediately.\n"
        "\n"
        "    -t, --show-touches\n"
        "        Enable \"show touches\" on start, disable on quit.\n"
        "        It only shows physical touches (not clicks from scrcpy).\n"
        "\n"
        "    -v, --version\n"
        "        Print the version of scrcpy.\n"
        "\n"
        "    --window-borderless\n"
        "        Disable window decorations (display borderless window).\n"
        "\n"
        "    --window-title text\n"
        "        Set a custom window title.\n"
        "\n"
        "    --window-x value\n"
        "        Set the initial window horizontal position.\n"
        "        Default is -1 (automatic).\n"
        "\n"
        "    --window-y value\n"
        "        Set the initial window vertical position.\n"
        "        Default is -1 (automatic).\n"
        "\n"
        "    --window-width value\n"
        "        Set the initial window width.\n"
        "        Default is 0 (automatic).\n"
        "\n"
        "    --window-height value\n"
        "        Set the initial window width.\n"
        "        Default is 0 (automatic).\n"
        "\n"
        "Shortcuts:\n"
        "\n"
        "    " CTRL_OR_CMD "+f\n"
        "        switch fullscreen mode\n"
        "\n"
        "    " CTRL_OR_CMD "+g\n"
        "        resize window to 1:1 (pixel-perfect)\n"
        "\n"
        "    " CTRL_OR_CMD "+x\n"
        "    Double-click on black borders\n"
        "        resize window to remove black borders\n"
        "\n"
        "    Ctrl+h\n"
        "    Middle-click\n"
        "        click on HOME\n"
        "\n"
        "    " CTRL_OR_CMD "+b\n"
        "    " CTRL_OR_CMD "+Backspace\n"
        "    Right-click (when screen is on)\n"
        "        click on BACK\n"
        "\n"
        "    " CTRL_OR_CMD "+s\n"
        "        click on APP_SWITCH\n"
        "\n"
        "    Ctrl+m\n"
        "        click on MENU\n"
        "\n"
        "    " CTRL_OR_CMD "+Up\n"
        "        click on VOLUME_UP\n"
        "\n"
        "    " CTRL_OR_CMD "+Down\n"
        "        click on VOLUME_DOWN\n"
        "\n"
        "    " CTRL_OR_CMD "+p\n"
        "        click on POWER (turn screen on/off)\n"
        "\n"
        "    Right-click (when screen is off)\n"
        "        power on\n"
        "\n"
        "    " CTRL_OR_CMD "+o\n"
        "        turn device screen off (keep mirroring)\n"
        "\n"
        "    " CTRL_OR_CMD "+Shift+o\n"
        "        turn device screen on\n"
        "\n"
        "    " CTRL_OR_CMD "+n\n"
        "       expand notification panel\n"
        "\n"
        "    " CTRL_OR_CMD "+Shift+n\n"
        "       collapse notification panel\n"
        "\n"
        "    " CTRL_OR_CMD "+c\n"
        "        copy device clipboard to computer\n"
        "\n"
        "    " CTRL_OR_CMD "+v\n"
        "        paste computer clipboard to device\n"
        "\n"
        "    " CTRL_OR_CMD "+Shift+v\n"
        "        copy computer clipboard to device\n"
        "\n"
        "    " CTRL_OR_CMD "+i\n"
        "        enable/disable FPS counter (print frames/second in logs)\n"
        "\n"
        "    Drag & drop APK file\n"
        "        install APK from computer\n"
        "\n",
        arg0,
        DEFAULT_BIT_RATE,
        DEFAULT_MAX_SIZE, DEFAULT_MAX_SIZE ? "" : " (unlimited)",
        DEFAULT_LOCAL_PORT);
}

static void
print_version(void) {
    fprintf(stderr, "scrcpy %s\n\n", SCRCPY_VERSION);

    fprintf(stderr, "dependencies:\n");
    fprintf(stderr, " - SDL %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
                                         SDL_PATCHLEVEL);
    fprintf(stderr, " - libavcodec %d.%d.%d\n", LIBAVCODEC_VERSION_MAJOR,
                                                LIBAVCODEC_VERSION_MINOR,
                                                LIBAVCODEC_VERSION_MICRO);
    fprintf(stderr, " - libavformat %d.%d.%d\n", LIBAVFORMAT_VERSION_MAJOR,
                                                 LIBAVFORMAT_VERSION_MINOR,
                                                 LIBAVFORMAT_VERSION_MICRO);
    fprintf(stderr, " - libavutil %d.%d.%d\n", LIBAVUTIL_VERSION_MAJOR,
                                               LIBAVUTIL_VERSION_MINOR,
                                               LIBAVUTIL_VERSION_MICRO);
}

int
main(int argc, char *argv[]) {
#ifdef __WINDOWS__
    // disable buffering, we want logs immediately
    // even line buffering (setvbuf() with mode _IOLBF) is not sufficient
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

#ifndef NDEBUG
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
#endif

    struct scrcpy_cli_args args = {
        .opts = SCRCPY_OPTIONS_DEFAULT,
        .help = false,
        .version = false,
    };

    if (!scrcpy_parse_args(&args, argc, argv)) {
        return 1;
    }

    if (args.help) {
        scrcpy_print_usage(argv[0]);
        return 0;
    }

    if (args.version) {
        print_version();
        return 0;
    }

    LOGI("scrcpy " SCRCPY_VERSION " <https://github.com/Genymobile/scrcpy>");

#ifdef SCRCPY_LAVF_REQUIRES_REGISTER_ALL
    av_register_all();
#endif

    if (avformat_network_init()) {
        return 1;
    }

    int res = scrcpy(&args.opts) ? 0 : 1;

    avformat_network_deinit(); // ignore failure

#if defined (__WINDOWS__) && ! defined (WINDOWS_NOCONSOLE)
    if (res != 0) {
        fprintf(stderr, "Press any key to continue...\n");
        getchar();
    }
#endif
    return res;
}
