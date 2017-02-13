var sim = {
  api: {
    uri: `https://brew-buddy-e99a0.firebaseio.com/brew_stage_event.json?auth=${process.env.FIREBASE_TOKEN}`,
    headers: {
      "Content-Type": "application/json"
    },
    payload: {
      "device_id": "SIMULATOR",
      "temperature": "{{temp}}",
      "brew_id": "-KW4wVrP8opJow2XKY3X",
      "brew_stage": "Initial Boil",
      "date_created": "{{date}}"
    }
  },
  lowTemp: 70,
  highTemp: 155,
  interval: 5000,
  maxDuration: 20000
};

module.exports = sim