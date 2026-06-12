## This is course material for Introduction to Artificial Intelligence
## Example code: cartpole_dqn.py
## Author: Kris Dong, Dima Nikiforov, Allen Y. Yang
##
## (c) Copyright 2020. Intelligent Racing Inc. Not permitted for commercial use

# Please make sure to install openAI gym module
# https://github.com/openai/gym
import random
import gym
import os
import numpy as np
from collections import deque
from keras.models import Sequential
from keras.layers import Dense
from tensorflow.keras.optimizers import Adam

import tensorflow as tf

import os

EPISODES = 500

class DQNAgent:
    def __init__(self, state_size, action_size):
        self.state_size = state_size
        self.action_size = action_size
        self.memory = deque(maxlen=10000)
        self.gamma = 0.95    # discount rate
        self.epsilon = 1.0  # exploration rate
        self.epsilon_min = 0.01
        self.epsilon_decay = 0.990
        self.learning_rate = 0.001
        self.model = self._build_model()

    def _build_model(self):
        # Neural Net for Deep-Q learning Model
        model = Sequential()
        model.add(Dense(12, input_dim=self.state_size, activation='relu'))
        model.add(Dense(12, activation='relu'))
        model.add(Dense(self.action_size, activation='linear'))
        model.compile(loss='mse',
                      optimizer=Adam(learning_rate=self.learning_rate))
        return model

    def remember(self, state, action, reward, next_state, done):
        self.memory.append((state, action, reward, next_state, done))

    def act(self, state):
        if np.random.rand() <= self.epsilon:
            return random.randrange(self.action_size)
        act_values = self.model.predict(state, verbose=0)
        return np.argmax(act_values[0])  # returns action

    def replay(self, batch_size):
        minibatch = random.sample(self.memory, batch_size)
        
        # Create empty arrays for batched updates
        states = np.zeros((batch_size, self.state_size))
        targets = np.zeros((batch_size, self.action_size))
        
        for i, (state, action, reward, next_state, done) in enumerate(minibatch):
            target = reward
            if not done:
                target = (reward + self.gamma *
                        np.amax(self.model.predict(next_state, verbose=0)[0]))
            
            # Only one step of forward-pass to get the current Q-values
            target_f = self.model.predict(state, verbose=0)
            target_f[0][action] = target
            
            # Accumulate the states and target Q-values for batched update
            states[i] = state
            targets[i] = target_f[0]
        
        # Batch update
        self.model.fit(states, targets, epochs=1, verbose=0)

        # Decay epsilon
        if self.epsilon > self.epsilon_min:
            self.epsilon *= self.epsilon_decay


    def load(self, name):
        self.model.load_weights(name)

    def save(self, name):
        self.model.save_weights(name)

    def generate_tflite(self, name="quant.tflite", e=1):
        x_values = random.sample(self.memory, min(len(self.memory), 500))
        def dataset(num_samples = len(x_values)):
            for i in range(num_samples):
                yield [x_values[i][0]]
        converter = tf.lite.TFLiteConverter.from_keras_model(self.model)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
        converter.inference_input_type = tf.int8
        converter.inference_output_type = tf.int8
        converter.representative_dataset = dataset
        tflite_model = converter.convert() 
        with open(f'./{name}', "wb") as f:
            f.write(tflite_model)


if __name__ == "__main__":
    env = gym.make('CartPole-v1')
    state_size = env.observation_space.shape[0]
    action_size = env.action_space.n
    agent = DQNAgent(state_size, action_size)
    # agent.load("./save/cartpole-dqn.h5")
    done = False
    batch_size = 32
    training_frequency = 10  # Train every 10 episodes
    print(f"GPUS: {tf.config.list_physical_devices('GPU')}")
    print('--------------- Starting Training')
    
    with tf.device('/GPU:0'):
        for e in range(EPISODES):
            state, _ = env.reset()
            state = np.reshape(state, [1, state_size])
            for time in range(500):
                # env.render()
                action = agent.act(state)
                next_state, reward, done, _, _ = env.step(action)
                # env.render()
                reward = reward if not done else -10
                next_state = np.reshape(next_state, [1, state_size])
                agent.remember(state, action, reward, next_state, done)
                state = next_state
                if done:
                    print("episode: {}/{}, score: {}, e: {:.2}"
                        .format(e, EPISODES, time, agent.epsilon))
                    break
                    
            # After every 'training_frequency' episodes, train the network
            if (e + 1) % training_frequency == 0 and len(agent.memory) > batch_size:
                print(f"Training at episode: {e+1}")
                for _ in range(training_frequency):  # Train for 'training_frequency' times
                    agent.replay(batch_size)
                print(f'Finished Training')

            # Continue with your checkpointing and TFLite generation
            if e % 10 == 0:
                agent.save(f"cartpole-dqn-unquant-{e}.h5")
                agent.generate_tflite(f'quant-{e}.tflite')
            agent.generate_tflite()


