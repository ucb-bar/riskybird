## This is course material for Introduction to Artificial Intelligence
## Example code: cartpole_ppo.py
## Author: Kris Dong, Dima Nikiforov
import numpy as np
import gym
import tensorflow as tf
from tensorflow import keras

# Hyperparameters
LEARNING_RATE = 0.002
GAMMA = 0.99
EPOCHS = 3
CLIP_EPSILON = 0.2
MAX_STEPS = 500
NUM_EPISODES = 1000

class PPOAgent:
    def __init__(self):
        self.policy = self.build_policy_network()

    def build_policy_network(self):
        model = keras.Sequential([
            keras.layers.Dense(128, activation='relu', input_shape=(4,)),
            keras.layers.Dense(2, activation='softmax')
        ])
        return model

    def get_action(self, state):
        probs = self.policy.predict(np.expand_dims(state, axis=0))[0]
        action = np.random.choice(2, p=probs)
        return action, probs[action]

    def compute_loss(self, advantages, old_probs, actions, rewards):
        def loss(y_true, y_pred):
            print("y_true shape: ", y_true.shape)  # Add this
            print("y_pred shape: ", y_pred.shape)  # Add this
            new_probs = tf.reduce_sum(y_true * y_pred, axis=1)
            print("old_probs shape:", old_probs.shape)

            ratio = new_probs / old_probs
            surr1 = ratio * advantages
            surr2 = tf.clip_by_value(ratio, 1.0 - CLIP_EPSILON, 1.0 + CLIP_EPSILON) * advantages
            return -tf.reduce_mean(tf.minimum(surr1, surr2))
        return loss


    def update(self, states, actions, rewards, old_probs):
        states = np.array(states)
        action_matrix = np.zeros((len(actions), 2))
        action_matrix[np.arange(len(actions)), actions] = 1

        returns = []
        R = 0
        for r in reversed(rewards):
            R = r + GAMMA * R
            returns.insert(0, R)
        returns = np.array(returns)
        returns = (returns - np.mean(returns)) / (np.std(returns) + 1e-5)
        
        action_probs = np.sum(action_matrix * self.policy.predict(states), axis=1)
        advantages = returns - action_probs

        loss_fn = self.compute_loss(advantages, old_probs, action_matrix, returns)
        self.policy.compile(optimizer=keras.optimizers.Adam(lr=LEARNING_RATE), loss=loss_fn)
        print("old_probs shape:", old_probs.shape)
        self.policy.fit(states, action_matrix, epochs=EPOCHS, verbose=0)


def main():
    env = gym.make('CartPole-v1')
    agent = PPOAgent()

    for episode in range(NUM_EPISODES):
        state, _ = env.reset()
        rewards, states, old_probs, actions = [], [], [], []

        for _ in range(MAX_STEPS):
            action, old_prob = agent.get_action(state)
            next_state, reward, done, _, _ = env.step(action)
            rewards.append(reward)
            states.append(state)
            old_probs.append(old_prob)
            actions.append(action)
            state = next_state
            if done:
                break

        agent.update(states, actions, rewards, old_probs)
        print(f"Episode {episode + 1}: Total Reward: {sum(rewards)}")

if __name__ == "__main__":
    main()