# Mender Deployment Scheduler

This Python script schedules deployments in Mender for a random number of devices with a random artifact at random intervals between 1 and 10 minutes.

## Prerequisites

- Python 3
- `requests`, `schedule`, and `python-dotenv` libraries

## Installation

1.  Install the required Python libraries:

    ```bash
    pip install requests schedule python-dotenv
    ```

## Configuration

1.  Create a `.env` file in the same directory as the script.
2.  Add the following lines to the `.env` file, replacing the placeholder values with your Mender server URL and Personal Access Token (PAT):

    ```
    MENDER_SERVER_URL="https://hosted.mender.io"
    MENDER_PAT="your_personal_access_token"
    ```

## How to get a Personal Access Token (PAT)

1.  Log in to your Mender account.
2.  Go to "My profile".
3.  Under "Personal Access Tokens", click "Create a token".
4.  Give the token a name and click "Create token".
5.  Copy the token and save it in a safe place. You will not be able to see it again.

## Usage

Run the script from the command line:

```bash
python mender_deployment_scheduler.py
```

The script will then run in the foreground, scheduling a new deployment at a random interval between 1 and 10 minutes. To stop the script, press `Ctrl+C`.
