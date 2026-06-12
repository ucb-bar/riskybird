## This is course material for Introduction to Artificial Intelligence
## Example code: cartpole_dqn.py
## Author: Kris Dong, Dima Nikiforov
##
## (c) Copyright 2023. 
import random
import gym
import numpy as np
import serial
import time
import cv2

EPISODES = 100
PORT = '/dev/ttyUSB0'

class ArduinoAgent:
    def __init__(self, state_size, action_size, port, baudrate=115200):
        self.state_size = state_size
        self.action_size = action_size
        self.port = port
        self.baudrate = baudrate
        self.connection = None

    def send_observation(self, observation):
        """Send the observation to the Arduino."""
        # Convert observation to string and send. Add newline for Arduino to recognize the end.
        # print(f"obs: {observation[0]}")
        self.connection.write(f"obs: {observation[0]}\n".encode())

    def read_action(self):
        """Read the action returned from the Arduino."""
        action_str = self.connection.readline().decode().strip()
        content = action_str.split('[')[1].split(']')[0]

        # Split the content by commas to get individual float strings
        int_strings = content.split(',')

        # Convert the string values to floats and create a NumPy array
        action = np.array([int(val.strip()) for val in int_strings])
        # print(f'action: {action[0]}')
        return action[0]

    def act(self, state):
        self.send_observation(state)
        action = self.read_action()
        return action

    def connect(self):
        self.connection = serial.Serial(self.port, self.baudrate)
        # Wait for the connection to establish
        time.sleep(2)

if __name__ == "__main__":
    env = gym.make('CartPole-v0', render_mode='rgb_array')
    state_size = env.observation_space.shape[0]
    action_size = env.action_space.n

    agent = ArduinoAgent(state_size, action_size, PORT)
    agent.connect()

    for e in range(EPISODES):
        state, _ = env.reset()
        state = np.reshape(state, [1, state_size])
        for time in range(200):
            action = agent.act(state)
            next_state, reward, done, _, _ = env.step(action)

            # Render using rgb_array mode and display using cv2
            frame = env.render()
            cv2.imshow('CartPole', frame)
            cv2.waitKey(1)  # This will display the frame for 1ms

            reward = reward if not done else -10
            next_state = np.reshape(next_state, [1, state_size])
            state = next_state
            if time % 10 == 0:
                print(f"Timestep {time}/200")
            if done:
                print("episode: {}/{}, score: {}".format(e, EPISODES, time))
                break

    cv2.destroyAllWindows()
