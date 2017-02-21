var simStrategy, lowTemp, highTemp, interval, totalDuration;

const simulateLinearIncreasing = (currentDuration) => {
  const tempRange = highTemp - lowTemp;
  const percentageComplete = currentDuration / totalDuration;
  
  return (tempRange * percentageComplete) + lowTemp;
};

const simulateLinearDecreasing = (currentDuration) => {
  const tempRange = highTemp - lowTemp;
  const percentageComplete = currentDuration / totalDuration;
  
  return highTemp - (tempRange * percentageComplete);
};

const simulateRandom = (currentDuration) => {
  return Math.floor(Math.random() * (highTemp - lowTemp + 1) + lowTemp);
}

const simulationStrategies = {
  'linear-increasing': simulateLinearIncreasing,
  'linear-decreasing': simulateLinearDecreasing,
  'random': simulateRandom
};

const initSimulator = (opts) => {
  simStrategy = opts.strategy;

  lowTemp = opts.lowTemp;
  highTemp = opts.highTemp;
  interval = opts.interval;
  totalDuration = opts.totalDuration;
};

const simulateTemp = (currentDuration) => {
  return simulationStrategies[simStrategy](currentDuration);
};

module.exports = {
  init: initSimulator,
  getTemp: simulateTemp,
}
