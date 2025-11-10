import requests
import json
import random
import schedule
import time
import os
from dotenv import load_dotenv

load_dotenv()

# Mender API endpoints
DEVICES_ENDPOINT = "/api/management/v2/devauth/devices"
ARTIFACTS_ENDPOINT = "/api/management/v2/deployments/artifacts"
DEPLOYMENTS_ENDPOINT = "/api/management/v1/deployments/deployments"

def get_devices(server_url, pat):
    """Gets all devices from the Mender server."""
    headers = {"Authorization": f"Bearer {pat}"}
    response = requests.get(f"{server_url}{DEVICES_ENDPOINT}", headers=headers)
    response.raise_for_status()
    return response.json()

def get_artifacts(server_url, pat):
    """Gets all artifacts from the Mender server."""
    headers = {"Authorization": f"Bearer {pat}"}
    response = requests.get(f"{server_url}{ARTIFACTS_ENDPOINT}", headers=headers)
    response.raise_for_status()
    return response.json()

def create_deployment(server_url, pat, artifact_name, devices):
    """Creates a deployment for the given artifact and devices."""
    headers = {"Authorization": f"Bearer {pat}", "Content-Type": "application/json"}
    deployment_name = f"deployment-{int(time.time())}"
    payload = {
        "name": deployment_name,
        "artifact_name": artifact_name,
        "devices": devices,
    }
    response = requests.post(
        f"{server_url}{DEPLOYMENTS_ENDPOINT}", headers=headers, data=json.dumps(payload)
    )
    response.raise_for_status()
    return deployment_name

def schedule_deployment(server_url, pat):
    """Schedules a deployment for a random number of devices and a random artifact."""
    try:
        print("Getting devices...")
        devices = get_devices(server_url, pat)
        if not devices:
            print("No devices found.")
            return

        print("Getting artifacts...")
        artifacts = get_artifacts(server_url, pat)
        if not artifacts:
            print("No artifacts found.")
            return

        num_devices_to_deploy = random.randint(1, len(devices))
        random_devices = random.sample(devices, num_devices_to_deploy)
        device_ids = [device["id"] for device in random_devices]

        random_artifact = random.choice(artifacts)
        artifact_name = random_artifact["name"]

        print(f"Creating deployment for {len(device_ids)} devices with artifact {artifact_name}...")
        deployment_name = create_deployment(server_url, pat, artifact_name, device_ids)
        print(f"Deployment '{deployment_name}' created successfully.")

    except requests.exceptions.RequestException as e:
        print(f"An error occurred: {e}")

def run_and_reschedule_deployment(server_url, pat):

    """Runs the deployment and reschedules it for a new random interval."""

    schedule_deployment(server_url, pat)

    new_interval = random.randint(1, 9)

    print(f"Scheduling next deployment in {new_interval} minutes.")

    schedule.every(new_interval).minutes.do(run_and_reschedule_deployment, server_url, pat)

    return schedule.CancelJob





if __name__ == "__main__":

    server_url = os.getenv("MENDER_SERVER_URL")

    pat = os.getenv("MENDER_PAT")



    if not server_url or not pat:

        print("MENDER_SERVER_URL and MENDER_PAT must be set in the .env file.")

    else:

        print("Starting Mender deployment scheduler with random intervals...")

        # Schedule the first run

        initial_interval = random.randint(1, 9)

        print(f"Scheduling first deployment in {initial_interval} minutes.")
        run_and_reschedule_deployment(server_url, pat)

        schedule.every(initial_interval).minutes.do(run_and_reschedule_deployment, server_url, pat)



        while True:

            schedule.run_pending()

            time.sleep(1)
